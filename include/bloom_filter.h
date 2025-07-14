#pragma once

#include <bitset>
#include <cstddef>
#include <utility>
#include <functional>

template<typename T, unsigned int count>
struct BloomFilterDefaultHasher {
    void operator()(const T& obj, std::size_t hashes[count])
    {
        std::size_t h1 = std::hash<T>{}(obj);
        std::size_t h2 = std::hash<std::size_t>{}(h1);
        for (unsigned int i = 0; i < count; ++i)
            hashes[i] = h1 + i * h2;
    }
};

template<typename Hasher, typename T, unsigned int count>
concept BloomFilterHasher = requires (Hasher h, T obj, std::size_t hashes[count])
{
    { h(obj, hashes) };
};

struct BloomFilterPolicy {
    unsigned int bits_per_item;
    unsigned int nr_hashes = 0;

    consteval unsigned int get_nr_hashes() const noexcept
    {
        if (nr_hashes <= 0)
            return (bits_per_item * 693 + 999) / 1000;
        else
            return nr_hashes;
    }
};

template<typename T, std::size_t capacity,
    template<typename U, unsigned int count> typename Hasher = BloomFilterDefaultHasher,
    BloomFilterPolicy policy = {.bits_per_item = 10}>
    requires BloomFilterHasher<Hasher<T, policy.get_nr_hashes()>, T, policy.get_nr_hashes()>
class BloomFilter {
public:
    using Hasher_ = Hasher<T, policy.get_nr_hashes()>;
    static constexpr unsigned int nr_hashes = policy.get_nr_hashes();

    explicit BloomFilter(Hasher_ hasher = Hasher_{}) : hasher(hasher) {}

    void insert(const T& item) noexcept
    {
        std::size_t hashes[nr_hashes];
        hasher(item, hashes);

        for (unsigned int i = 0; i < nr_hashes; ++i)
            bits[hashes[i] % capacity] = true;
    }

    bool probably_contains(const T& item) const
    {
        std::size_t hashes[nr_hashes];
        hasher(item, hashes);

        return [&]<std::size_t... I>(std::index_sequence<I...>)
            {
                return (bits[hashes[I] % capacity] && ...);
            }(std::make_index_sequence<nr_hashes>{});
    }

    inline bool definitely_missing(const T& item) const
    {
        return ! probably_contains(item);
    }

private:
    std::bitset<capacity> bits {};
    mutable Hasher_ hasher;
};
