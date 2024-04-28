#include <gtest/gtest.h>

#include "hash_table.h"

#include <string>
#include <algorithm>


template<typename T>
static T gen_sample_object();

template<typename TypeTuple>
class HashTableTest : public ::testing::Test {
protected:
    using Key   = std::tuple_element_t<0, TypeTuple>;
    using Value = std::tuple_element_t<1, TypeTuple>;

    bool test_insert_and_find(const Key& key, const Value& value)
    {
        auto it = this->hash_table.find(key);
        const bool will_insert = it == this->hash_table.end();

        if (will_insert) {
            auto [it, inserted] = this->hash_table.insert(std::make_pair(key, value));
            EXPECT_TRUE(inserted);
            EXPECT_EQ(it->first, key);
            EXPECT_EQ(it->second, value);
        } else {
            auto [it, inserted] = this->hash_table.insert(std::make_pair(key, value));
            EXPECT_FALSE(inserted);
            EXPECT_EQ(it->first, key);
            it->second = value;
        }

        this->hash_table[key] = value;
        EXPECT_EQ(this->hash_table[key], value) << "while the key is " << key;
        EXPECT_LE(this->hash_table.load_factor(), this->hash_table.max_load_factor() + 0.001F);

        return will_insert;
    }

    HashTable<Key, Value> hash_table;
};

using TestTypeTuples = ::testing::Types<std::tuple<std::string, int>, std::tuple<int, std::string>>;

TYPED_TEST_SUITE(HashTableTest, TestTypeTuples);

TYPED_TEST(HashTableTest, InsertionAndFinding)
{
    using Key   = std::tuple_element_t<0, TypeParam>;
    using Value = std::tuple_element_t<1, TypeParam>;

    int nr_unique_inserts = 0;
    for (int i = 0; i < 50; ++i) {
        Key key = gen_sample_object<Key>();
        Value value = gen_sample_object<Value>();
        nr_unique_inserts += this->test_insert_and_find(key, value);
    }
    EXPECT_EQ(nr_unique_inserts, this->hash_table.size()) << this->hash_table.size();

    for (auto [key, value] : this->hash_table) {
        this->test_insert_and_find(key, value);
        --nr_unique_inserts;
    }
    EXPECT_EQ(nr_unique_inserts, 0);
}

TYPED_TEST(HashTableTest, Erasing)
{
    using Key   = std::tuple_element_t<0, TypeParam>;
    using Value = std::tuple_element_t<1, TypeParam>;

    for (int i = 0; i < 50; ++i)
        this->hash_table[gen_sample_object<Key>()] = gen_sample_object<Value>();

    for (auto it = this->hash_table.begin(); it != this->hash_table.end();) {
        size_t size = this->hash_table.size();
        it = this->hash_table.erase(it);
        EXPECT_EQ(this->hash_table.size(), size - 1);
    }
    for (size_t i = 0; i < this->hash_table.bucket_count(); ++i)
        EXPECT_EQ(this->hash_table.bucket_size(i), 0);
}

TYPED_TEST(HashTableTest, Clearing)
{
    using Key   = std::tuple_element_t<0, TypeParam>;
    using Value = std::tuple_element_t<1, TypeParam>;

    for (int i = 0; i < 50; ++i)
        this->hash_table[gen_sample_object<Key>()] = gen_sample_object<Value>();

    this->hash_table.clear();
    EXPECT_EQ(this->hash_table.size(), 0);
    for (size_t i = 0; i < this->hash_table.bucket_count(); ++i)
        EXPECT_EQ(this->hash_table.bucket_size(i), 0);
}


template<> std::string gen_sample_object<std::string>()
{
    std::string str(rand() % 10 + 5, '\0');
    std::generate_n(str.begin(), str.size(), []() -> char
            {
                constexpr char alphanum[] =
                    "0123456789"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz";
                return alphanum[rand() % (sizeof(alphanum) / sizeof(alphanum[0]))];
            }
        );
    return str;
}

template<> int gen_sample_object<int>()
{
    return rand();
}

