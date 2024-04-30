#include "kmp_pattern_search.h"


std::size_t kmp_str_find_pattern(std::string_view str, std::string_view pat)
{
     return kmp_find_pattern(str.begin(), str.end(), pat.begin(), pat.size()) - str.begin();
}

std::size_t kmp_str_find(std::string_view str, std::string_view pat)
{
    std::size_t match = kmp_str_find_pattern(str, pat);
    return pat.size() + match <= str.size() ? match : std::string_view::npos;
}
