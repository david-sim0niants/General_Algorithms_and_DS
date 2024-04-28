#pragma once

#include <vector>
#include <forward_list>
#include <cstddef>
#include <utility>
#include <concepts>

#include "hash_table_policy.h"


/* Class representing a hash table. */
template<typename Key, typename Value, typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>, RehashPolicy RehashPolicy = Power2RehashPolicy>
class HashTable {
private:
    struct Node {
        std::pair<Key, Value> pair; // key-value pair
        Node *next; // the next node in a circular linked list

        explicit Node(std::pair<Key, Value>&& pair, Node *next = nullptr)
            : pair(std::move(pair)), next(next)
        {}
    };

    template<typename ContainerPointer = HashTable *, typename NodePointer = Node *>
    class Iterator_ {
    public:
        Iterator_() = default;

        explicit Iterator_(HashTable *container, bool begin = true) : container(container)
        {
            if (!begin)
                return;
            for (; index < container->buckets.size() && !container->buckets[index]; ++index);
            if (index < container->buckets.size())
                prev = container->buckets[index];
            else
                prev = nullptr;
        }

        Iterator_& operator++()
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

        Iterator_ operator++(int)
        {
            Iterator_ old = *this;
            ++*this;
            return old;
        }

        bool operator==(const Iterator_& other) const
        {
            return (container == other.container || !container || !other.container) && prev == other.prev;
        }

        bool operator!=(const Iterator_& other) const
        {
            return !(*this == other);
        }

        auto& operator*()
        {
            return prev->next->pair;
        }

        const auto& operator*() const
        {
            return prev->next->pair;
        }

        auto *operator->()
        {
            return &prev->next->pair;
        }

        const auto *operator->() const
        {
            return &prev->next->pair;
        }

        inline ContainerPointer get_container() const
        {
            return container;
        }

        inline size_t get_bucket_index() const
        {
            return index;
        }

    private:
        friend class HashTable;
        Iterator_(HashTable *container, size_t index, Node *prev)
            : container(container), index(index), prev(prev)
        {}

        ContainerPointer container = nullptr; // pointer to the hash table
        size_t index = 0; // current bucket index
        NodePointer prev = nullptr; // pointer to the node preceding the current node
    };

public:
    using Iterator = Iterator_<>;
    using ConstIterator = Iterator_<const HashTable *, const Node *>;

    HashTable() = default;
    explicit HashTable(Hash&& hasher, KeyEqual&& key_equal = KeyEqual(), RehashPolicy&& rehash_policy = RehashPolicy())
        : hasher(std::move(hasher)), key_equal(std::move(key_equal)), rehash_policy(std::move(rehash_policy))
    {}

    template<typename It>
    explicit HashTable(It begin, It end, Hash&& hasher, KeyEqual&&
            key_equal = KeyEqual(), RehashPolicy&& rehash_policy = RehashPolicy())
        : HashTable(std::move(hasher), std::move(key_equal), rehash_policy(std::move(rehash_policy)))
    {
        insert(begin, end);
    }

    ~HashTable()
    {
        clear();
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

    /* Insert a key-value pair. */
    std::pair<Iterator, bool> insert(std::pair<Key, Value>&& pair)
    {
        const auto hash = hasher(pair.first);
        size_t index = hash % buckets.size();

        // try to find a node with the key in the bucket first
        // and return without modifying if found
        Node *prev = find_node_in_bucket(pair.first, buckets[index]);
        if (prev)
            return std::make_pair(Iterator(this, index, prev), false);

        // check if rehash is needed and rehash
        auto [need_rehash, new_nr_buckets] = rehash_policy.need_rehash(buckets.size(), nr_elements, 1);
        if (need_rehash) {
            rehash(new_nr_buckets);
            index = hash % buckets.size(); // update the index after rehashing
        }

        // insert a node into bucket
        prev = insert_node_into_bucket(buckets[index], new Node(std::move(pair), nullptr));
        ++nr_elements;
        return std::make_pair(Iterator(this, index, prev), true);
    }

    inline std::pair<Iterator, bool> insert(const std::pair<Key, Value>& pair)
    {
        return insert(std::pair<Key, Value>(pair));
    }

    /* Insert a range of key-value pairs using iterators pointing to them. */
    template<typename It>
    void insert(It begin, It end)
    {
        auto [need_rehash, new_nr_buckets] = rehash_policy.need_rehash(
                buckets.size(), nr_elements, std::distance(begin, end));
        if (need_rehash)
            rehash();
        for (auto it = begin; it != end; ++it)
            find_or_insert__no_rehash(*it);
    }

    /* Find a key and return an iterator pointing to the key-value pair if found. */
    Iterator find(const Key& key)
    {
        size_t index = hasher(key) % buckets.size();
        return Iterator(this, index, find_node_in_bucket(key, buckets[index]));
    }

    /* Find a key and return an iterator pointing to the key-value pair if found. */
    const ConstIterator find(const Key& key) const
    {
        size_t index = hasher(key) % buckets.size();
        return ConstIterator(this, index, find_node_in_bucket(key, buckets[index]));
    }

    /* Erase an element pointed by the given iterator. */
    Iterator erase(const Iterator it)
    {
        Iterator next_iterator = it;
        if (it.prev->next == it.prev)
            ++next_iterator;
        delete remove_node_from_bucket(buckets[it.index], it.prev);
        --nr_elements;
        return next_iterator;
    }

    /* Erase an element with the given key. */
    size_t erase(const Key& key)
    {
        Iterator it = find(key);
        if (it == end())
            return 0;
        erase(it);
        return 1;
    }

    /* Erase all elements. */
    void clear()
    {
        for (Node *&bucket : buckets) {
            if (!bucket)
                continue;

            Node *node = bucket;
            do {
                Node *const next = node->next;
                delete node;
                node = next;
            } while (node != bucket);

            bucket = nullptr;
        }
        nr_elements = 0;
    }

    inline Iterator begin()
    {
        return Iterator(this, true);
    }

    inline Iterator end()
    {
        return Iterator(this, false);
    }

    inline const ConstIterator begin() const
    {
        return ConstIterator(this, true);
    }

    inline const ConstIterator end() const
    {
        return ConstIterator(this, false);
    }

    inline const ConstIterator cbegin() const
    {
        return ConstIterator(this, true);
    }

    inline const ConstIterator cend() const
    {
        return ConstIterator(this, false);
    }

    inline Value& operator[](const Key& key)
    {
        return insert(std::make_pair(key, Value())).first->second;
    }

    /* Return number of elements. */
    inline size_t size() const
    {
        return nr_elements;
    }

    /* Return if empty or not. */
    inline bool empty() const
    {
        return !nr_elements;
    }

    /* Reserve elements no less than nr_elements, possibly rehashing the table. */
    void reserve(size_t nr_elements)
    {
        size_t nr_buckets = rehash_policy.get_nr_buckets_for_elements(nr_elements);
        if (nr_buckets > buckets.size())
            rehash(nr_buckets);
    }

    /* Return bucket index for the given key. */
    inline size_t bucket(const Key& key) const
    {
        return hasher(key) % buckets.size();
    }

    /* Return number of buckets. */
    inline size_t bucket_count() const
    {
        return buckets.size();
    }

    /* Return size of the bucket at the given index. */
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

    /* Average number of elements per bucket. */
    float load_factor() const
    {
        return nr_elements / (float)buckets.size();
    }

    /* Max allowed average number of elements per bucket. */
    float max_load_factor() const
    {
        return rehash_policy.get_max_load_factor();
    }

private:
    /* Perform rehashing with the new number of buckets. */
    void rehash(size_t new_nr_buckets)
    {
        if (new_nr_buckets == 0) // illegal to have 0 number of buckets
            new_nr_buckets = 1;
        if (new_nr_buckets == buckets.size()) // no need to change anything
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
                // insert the node into another bucket
                insert_node_into_bucket(buckets[hasher(node->pair.first) % new_nr_buckets], node);
                node = old_next;
            } while (node != head);
        }
    }

    /* Find or insert a key-value pair without rehashing. */
    Node *find_or_insert__no_rehash(std::pair<Key, Value>&& pair)
    {
        const auto hash = hasher(pair.first);
        size_t index = hash % buckets.size();

        Node *prev = find_node_in_bucket(pair.first, buckets[index]);
        if (prev)
            return prev;
        prev = insert_node_into_bucket(buckets[index], new Node(std::move(pair), nullptr));
        ++nr_elements;
        return prev;
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

    static Node *remove_node_from_bucket(Node *&bucket, Node *prev_node)
    {
        if (prev_node->next == prev_node) {
            bucket = nullptr;
            return prev_node;
        }

        Node *const node = prev_node->next;
        if (node == bucket)
            bucket = node->next;
        prev_node->next = node->next;
        node->next = nullptr;
        return node;
    }

    std::vector<Node *> buckets = {nullptr};
    size_t nr_elements = 0;
    Hash hasher;
    KeyEqual key_equal;
    RehashPolicy rehash_policy;
};

