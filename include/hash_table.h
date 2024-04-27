#pragma once

#include <vector>
#include <forward_list>
#include <cstddef>
#include <utility>
#include <concepts>

#include "hash_table_policy.h"


template<typename Key, typename Value, typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>, RehashPolicy RehashPolicy = Power2RehashPolicy>
class HashTable {
private:
    struct Node {
        std::pair<Key, Value> pair;
        Node *next;

        explicit Node(std::pair<Key, Value>&& pair, Node *next = nullptr)
            : pair(std::move(pair)), next(next)
        {}
    };

public:
    class Iterator {
    public:
        Iterator() = default;

        explicit Iterator(HashTable *container, bool begin = true) : container(container)
        {
            if (!begin)
                return;
            for (; index < container->buckets.size() && !container->buckets[index]; ++index);
            if (index < container->buckets.size())
                prev = container->buckets[index];
            else
                prev = nullptr;
        }

        Iterator& operator++()
        {
            prev = prev->next;
            if (prev != container->buckets[index])
                return *this;
            for (++index; index < container->buckets.size() && !container->buckets[index]; ++index);
            if (index < container->buckets.size())
                prev = container->buckets[index];
            else
                prev = nullptr;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator old = *this;
            ++*this;
            return old;
        }

        bool operator==(const Iterator& other) const
        {
            return (container == other.container || !container || !other.container) && prev == other.prev;
        }

        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

        std::pair<Key, Value>& operator*()
        {
            return prev->next->pair;
        }

        const std::pair<Key, Value>& operator*() const
        {
            return prev->next->pair;
        }

        std::pair<Key, Value> *operator->()
        {
            return &prev->next->pair;
        }

        const std::pair<Key, Value> *operator->() const
        {
            return &prev->next->pair;
        }

    private:
        friend class HashTable;
        Iterator(HashTable *container, size_t index, Node *prev)
            : container(container), index(index), prev(prev)
        {}

        HashTable *container = nullptr;
        size_t index = 0;
        Node *prev = nullptr;
    };

public:
    HashTable() = default;
    explicit HashTable(Hash&& hasher, KeyEqual&& key_equal = KeyEqual(), RehashPolicy&& rehash_policy = RehashPolicy())
        : hasher(std::move(hasher)), key_equal(std::move(key_equal)), rehash_policy(std::move(rehash_policy))
    {}

    ~HashTable()
    {
        for (Node *bucket : buckets) {
            if (!bucket)
                continue;
            Node *node = bucket;
            do {
                Node *const next = node->next;
                delete node;
                node = next;
            } while (node != bucket);
        }
    }

    HashTable(HashTable&& other) noexcept
    {
        swap(other);
    }

    HashTable& operator=(HashTable&& other) noexcept
    {
        swap(other);
    }

    HashTable(const HashTable& other)
        : buckets(other.buckets), nr_elements(nr_elements), hasher(hasher), key_equal(key_equal), rehash_policy(rehash_policy)
    {
        for (Node *&bucket : buckets) {
            if (!bucket)
                continue;
            Node *new_bucket = new Node (std::pair(bucket->pair), bucket->next);
            Node *new_node = new_bucket;
            const Node *node = bucket;
            while (node->next != bucket) {
                new_node->next = new Node {std::pair(node->next->pair), nullptr};
                node = node->next;
                new_node = new_node->next;
            };
            new_node->next = new_bucket;
            bucket = new_bucket;
        }
    }

    HashTable& operator=(const HashTable& other)
    {
        HashTable copied {other};
        swap(copied);
    }

    void swap(HashTable& other) noexcept
    {
        using std::swap;
        swap(buckets, other.buckets);
        swap(nr_elements, other.nr_elements);
        swap(hasher, other.hasher);
        swap(key_equal, other.key_equal);
        swap(rehash_policy, other.rehash_policy);
    }

    std::pair<Iterator, bool> insert(std::pair<Key, Value>&& pair)
    {
        const auto hash = hasher(pair.first);
        size_t index = hash % buckets.size();

        Node *prev = find_node_in_bucket(pair.first, buckets[index]);
        if (prev)
            return std::make_pair(Iterator(this, index, prev), false);

        auto [need_rehash, new_nr_buckets] = rehash_policy.need_rehash(buckets.size(), nr_elements, 1);
        if (need_rehash) {
            rehash(new_nr_buckets);
            index = hash % buckets.size();
        }

        prev = insert_node_into_bucket(buckets[index], new Node(std::move(pair), nullptr));
        ++nr_elements;
        return std::make_pair(Iterator(this, index, prev), true);
    }

    inline std::pair<Iterator, bool> insert(const std::pair<Key, Value>& pair)
    {
        return insert(std::pair<Key, Value>(pair));
    }

    Iterator find(const Key& key) const
    {
        size_t index = hasher(key) % buckets.size();
        return Iterator(this, index, find_node_in_bucket(key, buckets[index]));
    }

    Iterator erase(const Iterator it)
    {
        Iterator next_iterator = it;
        if (it.prev->next == it.prev)
            ++next_iterator;
        remove_node_from_bucket(buckets[it.index], it.prev);
        --nr_elements;
        return next_iterator;
    }

    size_t erase(const Key& key)
    {
        Iterator it = find(key);
        if (it == end())
            return 0;
        erase(it);
        return 1;
    }

    inline Iterator begin()
    {
        return Iterator(this, true);
    }

    inline Iterator end()
    {
        return Iterator(this, false);
    }

    inline const Iterator cbegin() const
    {
        return Iterator(this, true);
    }

    inline const Iterator cend() const
    {
        return Iterator(this, false);
    }

    inline Value& operator[](const Key& key)
    {
        return insert(std::make_pair(key, Value())).first->second;
    }

    inline size_t size() const
    {
        return nr_elements;
    }

    void reserve(size_t nr_elements)
    {
        size_t nr_buckets = rehash_policy.get_nr_buckets_for_elements(nr_elements);
        if (nr_buckets > buckets.size())
            rehash(nr_buckets);
    }

    inline size_t bucket(const Key& key) const
    {
        return hasher(key) % buckets.size();
    }

    size_t bucket_size(size_t index) const
    {
        Node *node = buckets[index];
        if (!node)
            return 0;
        size_t size = 0;
        do {
            ++size;
            node = node->next;
        } while (node != buckets[index]);
        return size;
    }

    float load_factor() const
    {
        return nr_elements / (float)buckets.size();
    }

private:
    void rehash(size_t new_nr_buckets)
    {
        if (new_nr_buckets == 0)
            new_nr_buckets = 1;
        if (new_nr_buckets == buckets.size())
            return;

        std::vector<Node *> old_buckets (new_nr_buckets, nullptr);
        old_buckets.swap(buckets);

        for (size_t i = 0; i < old_buckets.size(); ++i) {
            Node *const head = old_buckets[i];
            if (!head)
                continue;
            Node *node = head;
            do {
                Node *const old_next = node->next;
                insert_node_into_bucket(buckets[hasher(node->pair.first) % new_nr_buckets], node);
                node = old_next;
            } while (node != head);
        }
    }

    Node *find_node_in_bucket(const Key& key, Node *bucket)
    {
        if (!bucket)
            return nullptr;

        Node *node = bucket;
        do {
            if (key_equal(key, node->next->pair.first))
                return node;
        } while (node != bucket);

        return nullptr;
    }

    static Node *insert_node_into_bucket(Node *&bucket, Node *new_node)
    {
        if (bucket) {
            new_node->next = bucket->next;
            bucket->next = new_node;
        } else {
            bucket = new_node;
            new_node->next = new_node;
        }
        return bucket;
    }

    static void remove_node_from_bucket(Node *&bucket, Node *prev_node)
    {
        if (prev_node->next == prev_node) {
            bucket = nullptr;
            return;
        }

        Node *const node = prev_node->next;
        if (node == bucket)
            bucket = node->next;
        prev_node->next = node->next;
    }

    std::vector<Node *> buckets = {nullptr};
    size_t nr_elements = 0;
    Hash hasher;
    KeyEqual key_equal;
    RehashPolicy rehash_policy;
};

