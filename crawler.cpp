#include "crawler.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <queue>
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;


Crawler::Crawler() : trie(std::make_shared<Trie>()) {};

Crawler::Node::Node() : isTerm(false), url("") {};

Crawler::Trie::Trie() : root(std::make_shared<Node>()) {} ;


// return top 5 most important pages
// O(sum of lengths of all urls + sum of lengths of all htmls)
std::vector<std::string> Crawler::query_top_five(std::string_view root_url, size_t max_htmls_cnt) {
    clear_all();

    std::queue<std::string> q;
    q.push(std::string{root_url});
    size_t iterations = 0;
    
    while(!q.empty() && (max_htmls_cnt == 0 || iterations < max_htmls_cnt)) {
        std::string url = q.front();
        q.pop();

        auto child_urls = parse_html(url, get_html(url));
        for (auto& child_url : child_urls) {
            add_hyperLink(url, child_url);

            if (!has_url(child_url)) {
                add_url(child_url);
                q.push(child_url);
            }
        }

        iterations++;
    }

    std::vector<std::string> top5Vector;
    for (const auto& elem : top5) {
        top5Vector.push_back(elem.second);
    }


    return top5Vector;
}

// return a list of pages which urls have same prefix
// O(sum of lengths of all urls in answer)
std::vector<std::string> Crawler::query_list_of_pages_with_given_prefix(std::string_view prefix) {
    return get_from_trie(trie->root, prefix, 0);
}


// check if the link has been added
// O(len(root_url))
bool Crawler::has_url(std::string_view root_url) {
    return urls.count(std::string{root_url});
}

// add newly found page url
// O(len(root_url))
void Crawler::add_url(std::string_view root_url) {
    urls.insert(std::string{root_url});
    put_in_trie(trie->root, root_url, 0);
}

// add the content of newly found page url
// O(len(root_url))
void Crawler::add_hyperLink(std::string_view root_url, std::string_view child_url) {
    if (top5.count({url_ranks[std::string{root_url}], std::string{root_url}})) {
        top5.erase({url_ranks[std::string{root_url}], std::string{root_url}});
    }

    url_ranks[std::string{child_url}]++;
    top5.insert({url_ranks[std::string{root_url}], std::string{root_url}});

    if (top5.size() > 5) {
        top5.erase(top5.begin());
    }
}

// clear all fields for future requests
void Crawler::clear_all() {
    url_ranks.clear();
    top5.clear();
    urls.clear();
    trie.reset(new Trie());
}


// Urls and HTML
// del https, http, www prefix, and add host, if there isn't one
std::string Crawler::standardize_url(const std::string_view url, const std::string_view host) {
    std::string link = std::string{ url };

    std::smatch match;
    if (std::regex_match(link, match, std::regex("(^https://)(.*)"))) {
        link = match[2];
    }
    if (std::regex_match(link, match, std::regex("(^http://)(.*)"))) {
        link = match[2];
    }
    if (std::regex_match(link, match, std::regex("(^www.)(.*)"))) {
        link = match[2];
    }
    if (link[0] == '/') {
        link = std::string{ host } + link;
    }

    return link;
}

// divide url into host and target
void Crawler::divide_url(const std::string_view url, std::string& host, std::string& target) {
    std::string link = standardize_url(url);
    std::smatch match;
    if (!std::regex_match(link, match, std::regex("([^/]*)(/.*)"))) {
        // only host
        host = url;
        target = "/";
    } else {
        host = match[1];
        target = match[2];
    }
}

// download html
std::string Crawler::get_html(const std::string_view url) {
    std::string host, target;
    divide_url(url, host, target);

    std::string html = "";
    try
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const resolvedDomain = resolver.resolve(host, /* port */ "80");

        beast::tcp_stream stream(ioc);
        stream.connect(resolvedDomain);

        http::request<http::string_body> req{http::verb::get, target, /*HTTP version*/ 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(stream, buffer, response);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if(ec && ec != beast::errc::not_connected) {
            throw beast::system_error{ec};
        }

        html = response.body();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: url: " << host << target << ' ' << e.what() << std::endl;
    }
    return html;
}

// get hrefs from html
std::vector<std::string> Crawler::parse_html(const std::string_view& url, const std::string_view& html) {
    std::string host, target;
    divide_url(url, host, target);

    const std::string open_tag = "<a href=\"";
    const std::string close_tag = "\">";

    size_t next_start = 0;
    size_t next_finish = 0;
    std::vector<std::string> links;
    while (true) {
        next_start = html.find(open_tag, next_finish);
        if (next_start == std::string::npos) { 
            return links;
        }

        next_finish = html.find(close_tag, next_start);
        if (next_finish == std::string::npos) { 
            return links;
        }

        std::string link(html, next_start + open_tag.size(), next_finish - next_start - open_tag.size());

        link = standardize_url(link, host);
        std::smatch match;
        // simple url structure check
        if (!std::regex_match(link, match, std::regex("^[A-Za-z0-9-_./&+,:;=?@#><%~\\[\\]]+$"))) {
            // bad url
            continue;
        }

        links.push_back(link);
    }

    return links;
}


// Trie
// put url in urls trie 
void Crawler::put_in_trie(std::shared_ptr<Node> vertex, std::string_view root_url, size_t index) {
    if (index == root_url.size()) {
        vertex->isTerm = true;
        vertex->url = root_url;
        return;
    }

    char symb = root_url[index];
    if (vertex->go[symb] == nullptr) {
        vertex->go[symb] = std::make_unique<Node>();
    }

    put_in_trie(vertex->go[symb], root_url, index + 1);
}

// get all urls in the trie subtree 
std::vector<std::string> Crawler::get_all_under_trie_vertex(std::shared_ptr<Node> vertex) {
    std::vector<std::string> res;
    if (vertex == nullptr) {
        return res;
    }

    if (vertex->isTerm) {
        res.push_back(vertex->url);
    }

    for (const auto& ways : vertex->go) { 
        auto v = get_all_under_trie_vertex(ways.second);
        res.insert(res.end(), v.begin(), v.end());
    }

    return res;

}

// get all urls collected for the prefix
std::vector<std::string> Crawler::get_from_trie(std::shared_ptr<Node> vertex, std::string_view prefix, size_t index) {
    if (index == prefix.size()) {
        return get_all_under_trie_vertex(vertex);
    }

    char symb = prefix[index];
    if (vertex->go[symb] == nullptr) {
        return std::vector<std::string>();
    }

    return get_from_trie(vertex->go[symb], prefix, index + 1);
}

