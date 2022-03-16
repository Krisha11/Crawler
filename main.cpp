#include "crawler.h"
#include <iostream>

// some examples

int main() {
    const std::string url = "https://www.cplusplus.com/reference/regex/";
    Crawler crawler = Crawler();

    for (auto& elem : crawler.query_top_five(url, /* max_html_cnt */ 15)) {
        std::cout << elem << ' ';
    }
    std::cout << "\n\n\n";


    for (auto& elem : crawler.query_list_of_pages_with_given_prefix("cplusplus.com")) {
        std::cout << elem << ' ';
    }
    std::cout << "\n\n\n";


    for (auto& elem : crawler.query_list_of_pages_with_given_prefix("cplusplus.com/info")) {
        std::cout << elem << ' ';
    }
    std::cout << "\n\n\n";
    return 0;
}

