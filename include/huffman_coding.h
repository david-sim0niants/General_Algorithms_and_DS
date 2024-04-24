#pragma once

#include <cstddef>
#include <variant>
#include <optional>
#include <memory>
#include <unordered_map>
#include <queue>
#include <istream>
#include <ostream>

#include "error.h"


template<typename Symbol>
class HuffmanTree {
private:
    struct Branches {
        std::unique_ptr<HuffmanTree> left, right;
    };

public:
    HuffmanTree(size_t freq, const Symbol& symbol)
        : freq(freq), content(symbol)
    {}

    HuffmanTree(std::unique_ptr<HuffmanTree>&& left, std::unique_ptr<HuffmanTree>&& right)
        :   freq((left ? left->freq : 0) + (right ? right->freq : 0)),
            content(Branches {std::move(left), std::move(right)})
    {
        if (!get_left() || !get_right())
            throw Error<HuffmanTree>("A non-symbol HuffmanTree node must have exactly two subtrees.");
    }

    inline size_t get_freq() const
    {
        return freq;
    }

    std::optional<Symbol> get_symbol() const
    {
        const Symbol *const p_symbol = std::get_if<Symbol>(&content);
        return p_symbol ? std::optional {*p_symbol} : std::nullopt;
    }

    inline HuffmanTree *get_left() const
    {
        const Branches *const p_branches = get_branches();
        return p_branches ? p_branches->left.get() : nullptr;
    }

    HuffmanTree *get_right() const
    {
        const Branches *const p_branches = get_branches();
        return p_branches ? p_branches->right.get() : nullptr;
    }

private:
    inline const Branches *get_branches() const
    {
        return std::get_if<Branches>(&content);
    }

    size_t freq;
    std::variant<Symbol, Branches> content;
};


template<typename Symbol>
std::unique_ptr<HuffmanTree<Symbol>> build_huffman_tree(const std::unordered_map<Symbol, size_t>& sym_freq);


template<typename SymbolIt>
std::unique_ptr<HuffmanTree<std::iter_value_t<SymbolIt>>> build_huffman_tree(SymbolIt begin, SymbolIt end)
{
    using Symbol = std::iter_value_t<SymbolIt>;
    std::unordered_map<Symbol, size_t> sym_freq;
    for (auto it = begin; it != end; ++it)
        ++sym_freq[*it];
    return build_huffman_tree(sym_freq);
}


template<typename Symbol>
std::unique_ptr<HuffmanTree<Symbol>> build_huffman_tree(const std::unordered_map<Symbol, size_t>& sym_freq)
{
    std::vector<HuffmanTree<Symbol> *> trees_storage;
    trees_storage.reserve(sym_freq.size());
    for (const auto& [symbol, freq] : sym_freq)
        trees_storage.push_back(new HuffmanTree<Symbol>(freq, symbol));

    struct Compare {
        bool operator()(HuffmanTree<Symbol> *left, HuffmanTree<Symbol> *right)
        {
            return left->get_freq() > right->get_freq();
        }
    };
    std::priority_queue<HuffmanTree<Symbol> *, std::vector<HuffmanTree<Symbol> *>, Compare>
        min_heap(Compare{}, trees_storage);

    while (min_heap.size() > 1) {
        HuffmanTree<Symbol> *left = min_heap.top(); min_heap.pop();
        HuffmanTree<Symbol> *right = min_heap.top(); min_heap.pop();

        min_heap.push(new HuffmanTree<Symbol>(
                    std::unique_ptr<HuffmanTree<Symbol>>(left),
                    std::unique_ptr<HuffmanTree<Symbol>>(right)));
    }

    HuffmanTree<Symbol> *tree = min_heap.top(); min_heap.pop();
    return std::unique_ptr<HuffmanTree<Symbol>> {tree};
}


template<typename Symbol>
using HuffmanTable = std::unordered_map<Symbol, std::string>;


template<typename Symbol>
void __build_huffman_table(const HuffmanTree<Symbol> *tree, HuffmanTable<Symbol>& table, std::string& codeword);


template<typename Symbol>
HuffmanTable<Symbol> build_huffman_table(const HuffmanTree<Symbol> *tree)
{
    if (!tree)
        return {};

    HuffmanTable<Symbol> table;
    std::string codeword;
    __build_huffman_table(tree, table, codeword);
    return table;
}


template<typename Symbol>
void __build_huffman_table(const HuffmanTree<Symbol> *tree, HuffmanTable<Symbol>& table, std::string& codeword)
{
    auto opt_symbol = tree->get_symbol();
    if (opt_symbol.has_value()) {
        const Symbol& symbol = *opt_symbol;
        table[symbol] = codeword;
        return;
    }

    codeword.push_back('0');
    __build_huffman_table(tree->get_left(), table, codeword);
    codeword.back() = '1';
    __build_huffman_table(tree->get_right(), table, codeword);
    codeword.pop_back();
}



template<typename Symbol>
class HuffmanEncoder {
public:
    HuffmanEncoder(std::ostream& os, HuffmanTable<Symbol>&& table)
        : os(os), table(std::move(table))
    {}

    HuffmanEncoder(std::ostream& os, const std::unique_ptr<HuffmanTree<Symbol>> &tree)
        : os(os), table(build_huffman_table(tree.get()))
    {}

    HuffmanEncoder(std::ostream& os, const HuffmanTree<Symbol> *tree)
        : os(os), table(build_huffman_table(tree))
    {}

    template<typename SymbolIt>
    HuffmanEncoder(std::ostream& os, SymbolIt begin, SymbolIt end)
        : os(os)
    {
        auto tree = build_huffman_tree(begin, end);
        table = build_huffman_table(tree.get());
    }

    ~HuffmanEncoder()
    {
        finalize();
    }

    void put_sym(const Symbol& symbol)
    {
        for (char c : table[symbol]) {
            if (!off_left--) {
                off_left = 7;
                os.put(unwritten_bits);
                unwritten_bits = 0;
            }
            unwritten_bits = (unwritten_bits << 1) + (c != '0');
        }
    }

    template<typename SymbolIt>
    void write_syms(SymbolIt begin, SymbolIt end)
    {
        for (auto it = begin; it != end; ++it)
            put_sym(*it);
    }

    void finalize()
    {
        if (off_left)
            os.put(unwritten_bits << off_left);
    }

private:
    std::ostream& os;
    HuffmanTable<Symbol> table;
    int off_left = 8;
    char unwritten_bits = 0;
};


template<typename Symbol>
class HuffmanDecoder {
public:
    HuffmanDecoder(std::istream& is, HuffmanTree<Symbol>& tree)
        : is(is), tree(tree)
    {}

    std::optional<Symbol> get_sym()
    {
        std::optional<Symbol> opt_symbol;
        while (!(opt_symbol = curr->get_symbol())) {
            if (!off_left--) {
                off_left = 7;
                unread_bits = is.get();
                if (is.eof())
                    return std::nullopt;
            }
            curr = unread_bits & (1 << off_left) ? curr->get_right() : curr->get_left();
        }
        curr = &tree;
        return opt_symbol;
    }

    template<typename SymbolIt>
    void read_syms(SymbolIt begin, SymbolIt end)
    {
        for (auto it = begin; it != end; ++it) {
            auto opt_symbol = get_sym();
            if (!opt_symbol)
                break;
            *it = *opt_symbol;
        }
    }

    void reset()
    {
        curr = &tree;
        off_left = 0;
        unread_bits = 0;
    }

private:
    std::istream& is;
    HuffmanTree<Symbol>& tree;
    HuffmanTree<Symbol> *curr = &tree;
    int off_left = 0;
    char unread_bits = 0;
};


template<typename Char>
class HuffmanBasicStringEncoder : public HuffmanEncoder<Char> {
public:
    HuffmanBasicStringEncoder(std::ostream& os, HuffmanTable<Char>&& table)
        : HuffmanEncoder<Char>(os, std::move(table))
    {}

    HuffmanBasicStringEncoder(std::ostream& os, const HuffmanTree<Char> *tree)
        : HuffmanEncoder<Char>(os, tree)
    {}

    template<typename CharIt>
    HuffmanBasicStringEncoder(std::ostream& os, CharIt begin, CharIt end)
        : HuffmanEncoder<Char>(os, begin, end)
    {}

    HuffmanBasicStringEncoder(std::ostream& os, const Char *str)
        : HuffmanEncoder<Char>(os, build_huffman_tree(count_freq(str)))
    {}

    HuffmanBasicStringEncoder(std::ostream& os, const std::basic_string<Char>& str)
        : HuffmanBasicStringEncoder<Char>(os, str.begin(), str.end())
    {}

    inline void putc(Char c)
    {
        HuffmanEncoder<Char>::put_sym(c);
    }

    inline void write(std::string_view s)
    {
        HuffmanEncoder<Char>::write_syms(s.begin(), s.end());
    }

private:
    static std::unordered_map<Char, size_t> count_freq(const Char *str)
    {
        std::unordered_map<Char, size_t> chr_freq;
        while (*str)
            ++chr_freq[*str++];
        return chr_freq;
    }
};


template<typename Char>
HuffmanBasicStringEncoder<Char>& operator<<(HuffmanBasicStringEncoder<Char>& encoder, Char c)
{
    encoder.putc(c);
    return encoder;
}

template<typename Char>
HuffmanBasicStringEncoder<Char>& operator<<(HuffmanBasicStringEncoder<Char>& encoder, std::string_view s)
{
    encoder.write_syms(s.begin(), s.end());
    return encoder;
}

template<typename Char, size_t size>
HuffmanBasicStringEncoder<Char>& operator<<(HuffmanBasicStringEncoder<Char>& encoder, const Char (&s)[size])
{
    encoder.write_syms(s, s + size);
    return encoder;
}


using HuffmanStringEncoder = HuffmanBasicStringEncoder<char>;
using HuffmanWStringEncoder = HuffmanBasicStringEncoder<wchar_t>;


template<typename Char>
class HuffmanBasicStringDecoder : HuffmanDecoder<Char> {
public:
    HuffmanBasicStringDecoder(std::istream& is, HuffmanTree<Char>& tree)
        : HuffmanDecoder<Char>(is, tree)
    {}

    inline std::optional<Char> getc()
    {
        return HuffmanDecoder<Char>::get_sym();
    }

    inline void read(Char *str, size_t size)
    {
        return HuffmanDecoder<Char>::read_syms(str, str + size);
    }

    inline void read(std::string& str)
    {
        str.clear();
        std::optional<Char> opt_c;
        while ((opt_c = getc()) && *opt_c)
            str.push_back(*opt_c);
    }
};


template<typename Char>
HuffmanBasicStringDecoder<Char>& operator>>(HuffmanBasicStringDecoder<Char>& decoder, Char& c)
{
    auto opt_symbol = decoder.get_sym();
    if (opt_symbol)
        c = *opt_symbol;
    return decoder;
}

template<typename Char, size_t size>
HuffmanBasicStringDecoder<Char>& operator>>(HuffmanBasicStringDecoder<Char>& decoder, Char (&s)[size])
{
    decoder.read(s, s + size);
    return decoder;
}

template<typename Char>
HuffmanBasicStringDecoder<Char>& operator>>(HuffmanBasicStringDecoder<Char>& decoder, std::string& str)
{
    decoder.read(str);
    return decoder;
}


using HuffmanStringDecoder = HuffmanBasicStringDecoder<char>;
using HuffmanWStringDecoder = HuffmanBasicStringDecoder<wchar_t>;

