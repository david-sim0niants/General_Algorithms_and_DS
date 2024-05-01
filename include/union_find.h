#pragma once

#include <utility>
#include <vector>
#include <concepts>

template<std::integral Index = int>
class UnionFind {
public:
    explicit UnionFind(std::size_t size = 0)
        : parent(size), rank(size, 1)
    {
        for (std::size_t i = 0; i < size; ++i)
            parent[i] = static_cast<Index>(i);
    }

    Index find(Index p) const
    {
        while (p != parent[p])
            p = parent[p];
        return p;
    }

    Index find(Index p)
    {
        return parent[p] = std::as_const(*this).find(p);
    }

    void merge(Index x, Index y)
    {
        x = find(x); y = find(y);
        if (rank[x] < rank[y])
            std::swap(x, y);
        rank[y] = rank[x] += !!(rank[x] == rank[y]);
        parent[y] = parent[x] = x;
    }

    inline bool connected(Index x, Index y) const
    {
        return find(x) == find(y);
    }

    inline std::size_t size() const
    {
        return parent.size();
    }

    void resize(std::size_t size)
    {
        const std::size_t prev_size = parent.size();
        parent.resize(size);
        rank.resize(size);
        for (std::size_t i = prev_size; i < size; ++i) {
            parent[i] = static_cast<Index>(i);
            rank[i] = 1;
        }
    }

private:
    std::vector<Index> parent, rank;
};
