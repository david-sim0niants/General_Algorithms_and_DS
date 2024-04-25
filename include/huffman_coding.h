#pragma once

/*
 * Huffman coding: Huffman Tree, Codeword table, Encoder and Decoder
 * */

#include <cstddef>
#include <variant>
#include <optional>
#include <memory>
#include <unordered_map>
#include <queue>
#include <istream>
#include <ostream>

#include "error.h"


/* Class representing the Huffman tree. */
template<typename Symbol>
class HuffmanTree {
private:
    struct Branches {
        // Left and right subtrees which are stored as unique pointers.
        std::unique_ptr<HuffmanTree> left, right;
    };

public:
    /* Construct a leaf (symbol) Huffman tree node. */
    HuffmanTree(size_t freq, const Symbol& symbol)
        : freq(freq), content(symbol)
    {}

    /* Construct a non-leaf Huffman tree node by merging left and right subtrees. */
    HuffmanTree(std::unique_ptr<HuffmanTree>&& left, std::unique_ptr<HuffmanTree>&& right)
        :   freq((left ? left->freq : 0) + (right ? right->freq : 0)),
            content(Branches {std::move(left), std::move(right)})
    {
        if (!get_left() || !get_right())
            throw Error<HuffmanTree>("A non-leaf HuffmanTree node must have exactly two subtrees.");
    }

    /* Get frequency of the node. */
    inline size_t get_freq() const
    {
        return freq;
    }

    /* Get symbol if the node is a leaf node (contains symbol); otherwise, nothing. */
    std::optional<Symbol> get_symbol() const
    {
        const Symbol *const p_symbol = std::get_if<Symbol>(&content);
        return p_symbol ? std::optional {*p_symbol} : std::nullopt;
    }

    /* Get left subtree if the node is a non-leaf node; otherwise, nothing. */
    inline HuffmanTree *get_left() const
    {
        const Branches *const p_branches = get_branches();
        return p_branches ? p_branches->left.get() : nullptr;
    }

    /* Get right subtree if the node is a non-leaf node; otherwise, nothing. */
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

    size_t freq; /* The frequency of the node: for a leaf (symbol) node, it stores
                    the frequency of the symbol; otherwise, it stores the cumulative
                    frequency of all leaf (symbol) nodes descending from this node. */
    std::variant<Symbol, Branches> content; /* Either a symbol for a leaf node, or a left and right subtree pair. */
};


template<typename Symbol>
std::unique_ptr<HuffmanTree<Symbol>> build_huffman_tree(const std::unordered_map<Symbol, size_t>& sym_freq);


/* Build an optimal Huffman tree for the provided stream of symbols. */
template<typename SymbolIt>
std::unique_ptr<HuffmanTree<std::iter_value_t<SymbolIt>>> build_huffman_tree(SymbolIt begin, SymbolIt end)
{
    using Symbol = std::iter_value_t<SymbolIt>;
    std::unordered_map<Symbol, size_t> sym_freq;
    for (auto it = begin; it != end; ++it)
        ++sym_freq[*it]; // measure the frequency of each appearing symbol
    return build_huffman_tree(sym_freq);
}


/* Build Huffman tree from the provided symbol frequency map. */
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


/* Type representing Huffman table that maps each symbol to its codeword. */
template<typename Symbol>
using HuffmanTable = std::unordered_map<Symbol, std::string>;


template<typename Symbol>
void __build_huffman_table(const HuffmanTree<Symbol> *tree, HuffmanTable<Symbol>& table, std::string& codeword);


/* Build Huffman table by traversing through the tree. */
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


/* Internal Huffman table builder function that actually does the traversal through the tree. */
template<typename Symbol>
void __build_huffman_table(const HuffmanTree<Symbol> *tree, HuffmanTable<Symbol>& table, std::string& codeword)
{
    auto opt_symbol = tree->get_symbol();
    if (opt_symbol.has_value()) {
        const Symbol& symbol = *opt_symbol;
        table[symbol] = codeword;
        return;
    }

    codeword.push_back('0'); // left path corresponds to 0
    __build_huffman_table(tree->get_left(), table, codeword);
    codeword.back() = '1'; // right path corresponds to 1
    __build_huffman_table(tree->get_right(), table, codeword);
    codeword.pop_back();
}


/* Huffman encoder class that deals with compression of the supplied data. */
template<typename Symbol>
class HuffmanEncoder {
public:
    /* Construct Huffman encoder from the table. */
    HuffmanEncoder(std::ostream& os, HuffmanTable<Symbol>&& table)
        : os(os), table(std::move(table))
    {}

    /* Construct Huffman encoder from the tree. */
    HuffmanEncoder(std::ostream& os, const std::unique_ptr<HuffmanTree<Symbol>> &tree)
        : os(os), table(build_huffman_table(tree.get()))
    {}

    /* Construct Huffman encoder from the tree. */
    HuffmanEncoder(std::ostream& os, const HuffmanTree<Symbol> *tree)
        : os(os), table(build_huffman_table(tree))
    {}

    /* Construct Huffman encoder with optimal codeword table for the provided symbol stream. */
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

    /* Encode one symbol and write to the output stream. */
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

    /* Encode symbol stream and write to the output stream. */
    template<typename SymbolIt>
    void write_syms(SymbolIt begin, SymbolIt end)
    {
        for (auto it = begin; it != end; ++it)
            put_sym(*it);
    }

    /* Finalize the stream by writing last unwritten bits followed by zero bits. */
    void finalize()
    {
        if (off_left & 7)
            os.put(unwritten_bits << off_left);
        off_left = 8;
    }

private:
    std::ostream& os; /* The output stream where the encoded data is written to. */
    HuffmanTable<Symbol> table; /* Huffman codeword table. */
    int off_left = 8; /* Offset left to fill to send 8 bits to the stream. */
    char unwritten_bits = 0; /* Unwritten bits of last encoded data. */
};


/* Huffman decoder class that deals with decompression of the compressed data. */
template<typename Symbol>
class HuffmanDecoder {
public:
    /* Construct Huffman decoder from the tree (the tree must be alive throughout the lifetime of the decoder). */
    HuffmanDecoder(std::istream& is, HuffmanTree<Symbol>& tree)
        : is(is), tree(tree)
    {}

    /* Read one symbol from the input stream. */
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

    /* Read multiple symbols from the input stream. */
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

    /* Reset the decoder state. */
    void reset()
    {
        curr = &tree;
        off_left = 0;
        unread_bits = 0;
    }

private:
    std::istream& is; /* The input stream from which the encoded data is read. */
    HuffmanTree<Symbol>& tree; /* Reference to the Huffman tree. */
    HuffmanTree<Symbol> *curr = &tree; /* Last visited Huffman tree node during decoding. */
    int off_left = 0; /* Offset of unread bits. */
    char unread_bits = 0; /* Unread bits. */
};


/* Huffman basic string encoder designed for compressing character streams. */
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


/* Huffman basic string decoder designed for decompressing compressed character streams. */
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

