#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Crawler {
public:
    Crawler();

    // return top 5 most important pages
    // max_htmls_cnt = 0 for infinity
    std::vector<std::string> query_top_five(std::string_view root_url, size_t max_htmls_cnt = 0);

    // return a list of pages which urls have same prefix
    std::vector<std::string> query_list_of_pages_with_given_prefix(std::string_view prefix);

// private: TODO!!
public:
    struct Node {
        std::unordered_map<char, std::shared_ptr<Node>> go;
        bool isTerm;
        std::string url;

        Node();
    };

    struct Trie {
        std::shared_ptr<Node> root;

        Trie();
    };


    // check if the link has been added
    bool has_url(std::string_view root_url);

    // add newly found page url
    // O(len(root_url))
    void add_url(std::string_view root_url);

    // add the content of newly found page url
    void add_hyperLink(std::string_view root_url, std::string_view child_url);

    // clear all fields for future requests
    void clear_all();


    // Urls and HTML
    // del https, http, www prefix, and add host, if there isn't one
    std::string standardize_url(const std::string_view url, const std::string_view host = "");

    //  divide url into host and target
    void divide_url(const std::string_view url, std::string& host, std::string& target);

    // download html
    std::string get_html(const std::string_view url);

    // get hrefs from html
    std::vector<std::string> parse_html(const std::string_view& url, const std::string_view& html);

    // Trie
    // put url in urls trie 
    void put_in_trie(std::shared_ptr<Node> vertex, std::string_view root_url, size_t index);

    // get all urls in the trie subtree 
    std::vector<std::string> get_all_under_trie_vertex(std::shared_ptr<Node> vertex);

    // get all urls collected for the prefix
    std::vector<std::string> get_from_trie(std::shared_ptr<Node> vertex, std::string_view prefix, size_t index);

private:
    std::unordered_map<std::string, int> url_ranks;
    std::set<std::pair<int, std::string>> top5;
    std::unordered_set<std::string> urls;
    std::shared_ptr<Trie> trie;
};
