#include <gtest/gtest.h>

#include "bloom_filter.h"

TEST(BloomFilterTest, BasicTest)
{
    BloomFilter<int, 1000> filter;
    filter.insert(15);
    EXPECT_TRUE(filter.probably_contains(15));
    EXPECT_FALSE(filter.probably_contains(30));
    EXPECT_FALSE(filter.definitely_missing(15));
    EXPECT_TRUE(filter.definitely_missing(40));
}
