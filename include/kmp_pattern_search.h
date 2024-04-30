#pragma once


#include <vector>
#include <type_traits>
#include <concepts>
#include <string_view>
#include <functional>


template<std::random_access_iterator PatIt, std::random_access_iterator LpsIt,
    typename Size = std::iter_value_t<LpsIt>,
    typename ChrEqual = std::equal_to<std::iter_value_t<PatIt>>,
    typename = std::enable_if_t<std::is_integral_v<std::iter_value_t<LpsIt>>>>
void build_lps(PatIt pattern, LpsIt lps, Size size,
        const ChrEqual& chr_equal = ChrEqual())
{
    lps[0] = 0;
    for (Size i = 1; i < size; ++i) {
        auto j = lps[i - 1];
        while (true) {
            if (chr_equal(pattern[i], pattern[j])) {
                lps[i] = j + 1;
                break;
            }
            if (j == 0) {
                lps[i] = 0;
                break;
            }
            j = lps[j];
        }
    }
}


template<std::input_iterator StrIt, std::random_access_iterator PatIt,
    std::random_access_iterator LpsIt, typename Size = std::iter_value_t<LpsIt>,
    typename ChrEqual = std::equal_to<std::iter_value_t<PatIt>>,
    typename = std::enable_if_t<std::is_same_v<std::iter_value_t<StrIt>, std::iter_value_t<PatIt>>
        && std::is_integral_v<std::iter_value_t<LpsIt>>
        && std::is_convertible_v<Size, std::iter_value_t<LpsIt>> > >
std::pair<StrIt, Size> kmp_find_pattern_raw(StrIt str_beg, StrIt str_end, PatIt pattern,
        LpsIt lps, Size pattern_size, const ChrEqual& chr_equal = ChrEqual())
{
    Size j = 0;
    StrIt it = str_beg;
    for (; it != str_end && j < pattern_size; ++it) {
        while (true) {
            if (chr_equal(*it, pattern[j])) {
                ++j;
                break;
            }
            if (j == 0)
                break;
            j = static_cast<std::iter_value_t<LpsIt>>(lps[j]);
        }
    }
    return std::make_pair(it, j);
}

template<std::input_iterator StrIt, std::random_access_iterator PatIt,
    typename Size, typename ChrEqual = std::equal_to<std::iter_value_t<PatIt>>,
    typename = std::enable_if_t<std::is_same_v<std::iter_value_t<StrIt>, std::iter_value_t<PatIt>>>>
std::pair<StrIt, Size> kmp_find_pattern_raw(StrIt str_beg, StrIt str_end,
        PatIt pattern, Size pattern_size, const ChrEqual& chr_equal = ChrEqual())
{
    std::vector<Size> lps (pattern_size);
    build_lps(pattern, lps.begin(), pattern_size, chr_equal);
    return kmp_find_pattern_raw(str_beg, str_end, pattern, lps.begin(), pattern_size, chr_equal);
}


template<std::random_access_iterator StrIt, std::random_access_iterator PatIt,
    std::random_access_iterator LpsIt, typename Size = std::iter_value_t<LpsIt>,
    typename ChrEqual = std::equal_to<std::iter_value_t<PatIt>>,
    typename = std::enable_if_t<std::is_same_v<std::iter_value_t<StrIt>, std::iter_value_t<PatIt>>
        && std::is_integral_v<std::iter_value_t<LpsIt>>
        && std::is_convertible_v<Size, std::iter_value_t<LpsIt>> > >
StrIt kmp_find_pattern(StrIt str_beg, StrIt str_end, PatIt pattern, LpsIt lps,
        Size size, const ChrEqual& chr_equal = ChrEqual())
{
    auto [end_it, match_len] = kmp_find_pattern_raw(str_beg, str_end, pattern, lps, size, chr_equal);
    return end_it - match_len;
}

template<std::random_access_iterator StrIt, std::random_access_iterator PatIt,
    typename Size, typename ChrEqual = std::equal_to<std::iter_value_t<PatIt>>,
    typename = std::enable_if_t<std::is_same_v<std::iter_value_t<StrIt>, std::iter_value_t<PatIt>>>>
StrIt kmp_find_pattern(StrIt str_beg, StrIt str_end,
        PatIt pattern, Size pattern_size, const ChrEqual& chr_equal = ChrEqual())
{
    auto [end_it, match_len] = kmp_find_pattern_raw(str_beg, str_end, pattern, pattern_size, chr_equal);
    return end_it - match_len;
}


std::size_t kmp_str_find_pattern(std::string_view str, std::string_view pat);

std::size_t kmp_str_find(std::string_view str, std::string_view pat);
