#include <gtest/gtest.h>

#include "kmp_pattern_search.h"


TEST(KmpPatternSearch, StrFind)
{
    std::string s = "Some random thing";
    EXPECT_EQ(kmp_str_find(s, "thing"), s.find("thing"));
    EXPECT_EQ(kmp_str_find(s, "rndom"), s.find("rndom"));
}
