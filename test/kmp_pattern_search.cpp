#include <gtest/gtest.h>

#include "kmp_pattern_search.h"


TEST(KmpPatternSearch, StrFind)
{
    std::string s = "Some random thing";
    EXPECT_EQ(kmp_str_find(s, "thing"), s.find("thing"));
    EXPECT_EQ(kmp_str_find(s, "rndom"), s.find("rndom"));
}

TEST(KmpPatternSearch, SeqFind)
{
    size_t seq_size = rand() % 100 + 50;
    std::vector<int> seq (seq_size);
    for (size_t i = 0; i < seq_size; ++i)
        seq[i] = rand();
    size_t i_pat = rand() % seq_size;
    size_t len = rand() % (seq_size - i_pat);

    auto it = kmp_find_pattern(seq.begin(), seq.end(), seq.begin() + i_pat, len);
    EXPECT_EQ(it - seq.begin(), i_pat);
}

