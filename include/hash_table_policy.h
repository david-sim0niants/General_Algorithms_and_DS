#pragma once

#include <cstddef>
#include <utility>
#include <concepts>
#include <bit>
#include <cmath>


template<typename T>
concept RehashPolicy = requires (T&& policy, size_t nr_buckets, size_t nr_elements, size_t nr_inserts)
{
    { policy.need_rehash(nr_buckets, nr_elements, nr_inserts) } -> std::same_as<std::pair<bool, size_t>>;
    { policy.get_nr_buckets_for_elements(nr_elements) } -> std::same_as<size_t>;
    { policy.next_nr_buckets(nr_buckets) } -> std::same_as<size_t>;
    { policy.get_max_load_factor() } -> std::same_as<float>;
};


class Power2RehashPolicy {
public:
    explicit Power2RehashPolicy(float max_load_factor = 0.75F)
        : max_load_factor(max_load_factor)
    {}

    std::pair<bool, size_t> need_rehash(size_t nr_buckets, size_t nr_elements, size_t nr_inserts) const
    {
        const size_t min_nr_buckets = get_nr_buckets_for_elements(nr_elements + nr_inserts);
        if (nr_buckets >= min_nr_buckets)
            return std::make_pair(false, nr_buckets);
        else
            return std::make_pair(true, next_nr_buckets(min_nr_buckets));
    }

    inline size_t get_nr_buckets_for_elements(size_t nr_elements) const
    {
        return static_cast<size_t>(std::ceil(nr_elements / max_load_factor));
    }

    inline size_t next_nr_buckets(size_t nr_buckets) const
    {
        return nr_buckets > penult_pow2 ? nr_buckets : std::bit_ceil(nr_buckets);
    }

    inline float get_max_load_factor() const
    {
        return max_load_factor;
    }

private:
    float max_load_factor;

    static constexpr size_t penult_pow2 = ((std::numeric_limits<size_t>::max() >> 1) + 1);
};

