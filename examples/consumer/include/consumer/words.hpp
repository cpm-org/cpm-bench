#pragma once

#include <string>
#include <string_view>
#include <vector>

// The words of `text`: what is between runs of spaces.
std::vector<std::string_view> split_words(std::string_view text);

// `words` with one space between each two.
std::string join_words(const std::vector<std::string_view>& words);
