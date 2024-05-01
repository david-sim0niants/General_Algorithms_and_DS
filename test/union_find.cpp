#include <gtest/gtest.h>

#include "union_find.h"

TEST(UnionFindTest, InitResizing)
{
    UnionFind uf (rand() % 100 + 50);
    uf.resize(uf.size() + rand() % 20);
    for (int i = 0; i < uf.size(); ++i)
        EXPECT_EQ(uf.find(i), i);
}

TEST(UnionFindTest, MergeFind)
{
    UnionFind uf (rand() % 100 + 50);
    int merge_count = uf.size();
    while (merge_count--) {
        int i = rand() % uf.size();
        int j = rand() % uf.size();
        int k = rand() % uf.size();
        uf.merge(i, j);
        EXPECT_TRUE(uf.connected(i, j)) << "find(i)=" << uf.find(i) << " find(j)=" << uf.find(j);
        uf.merge(j, k);
        EXPECT_TRUE(uf.connected(j, k)) << "find(j)=" << uf.find(j) << " find(k)=" << uf.find(k);

        EXPECT_TRUE(uf.connected(k, i)) << "find(k)=" << uf.find(k) << " find(i)=" << uf.find(i);
    }
}
