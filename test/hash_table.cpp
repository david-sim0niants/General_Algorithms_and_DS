#include <gtest/gtest.h>

#include "hash_table.h"

#include <string>

TEST(HashTable, Insertion)
{
    HashTable<std::string, int> hash_table;
    for (int i = 0; i < 50; ++i) {
        std::string key (3, 'a');
        key[0] = 'a' + rand() % 26;
        key[1] = 'A' + rand() % 26;
        key[2] = 'a' + rand() % 26;
        int val = rand();
        hash_table[key] = val;
        EXPECT_EQ(hash_table[key], val) << "while the key is " << key;
    }
    EXPECT_EQ(hash_table.size(), 50);
}
