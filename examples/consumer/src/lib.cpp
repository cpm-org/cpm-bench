#include <consumer/words.hpp>

std::vector<std::string_view> split_words(std::string_view text) {
    std::vector<std::string_view> words;
    std::size_t start = text.find_first_not_of(' ');
    while (start != std::string_view::npos) {
        const std::size_t end = text.find(' ', start);
        words.push_back(text.substr(start, end - start));
        start = end == std::string_view::npos ? end : text.find_first_not_of(' ', end);
    }
    return words;
}

std::string join_words(const std::vector<std::string_view>& words) {
    std::string text;
    for (const std::string_view word : words) {
        if (!text.empty()) text += ' ';
        text += word;
    }
    return text;
}
