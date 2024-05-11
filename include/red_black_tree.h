#pragma once

#include <utility>
#include <type_traits>

template<typename T>
class RedBlackTree {
private:
    enum class Color : char {
        Black = 0, Red = 1
    };

    enum class Direction {
        Left = 0, Right = 1
    };

public:
    RedBlackTree() = default;

    RedBlackTree(T&& value) : value(std::move(value))
    {}

    RedBlackTree(const T& value) : value(value)
    {}

    void insert_left(RedBlackTree *node)
    {
        insert__common(node);
        left = node;
        node->balance();
    }

    void insert_right(RedBlackTree *node)
    {
        insert__common(node);
        right = node;
        node->balance();
    }

    inline bool is_black() const
    {
        return color == Color::Black;
    }

    inline bool is_red() const
    {
        return color == Color::Red;
    }

    inline bool is_left() const
    {
        return !parent || parent->left == this;
    }

    inline bool is_right() const
    {
        return !parent || parent->right == this;
    }

    inline bool is_root() const
    {
        return parent == nullptr;
    }

    inline RedBlackTree *get_left() const
    {
        return left;
    }

    inline RedBlackTree *get_right() const
    {
        return right;
    }

    inline RedBlackTree *get_parent() const
    {
        return parent;
    }

    inline RedBlackTree *get_root()
    {
        RedBlackTree *node = this;
        while (!node->is_root())
            node = node->get_parent();
        return node;
    }

private:
    template<Direction dir>
    RedBlackTree(T&& value, Color color = Color::Red, RedBlackTree *parent = nullptr)
        : value(std::move(value)), color(color), left(nullptr), right(nullptr), parent(parent)
    {
        if (parent) {
            if constexpr (dir == Direction::Left)
                parent->left = this;
            else
                parent->right = this;
        }
    }

    inline static constexpr Direction opposite(Direction dir)
    {
        return static_cast<Direction>( ! static_cast<std::underlying_type_t<Direction>>(dir));
    }

    void insert__common(RedBlackTree *node)
    {
        node->parent = this;
        node->color = Color::Red;
    }

    template<Direction dir>
    void rotate()
    {
        RedBlackTree *&opposite_dir_child = get_child<opposite(dir)>();
        if (parent)
            parent->get_child(get_dir()) = opposite_dir_child;
        opposite_dir_child->parent = parent;
        parent = opposite_dir_child;
        opposite_dir_child = opposite_dir_child->get_child<dir>();
        if (opposite_dir_child)
            opposite_dir_child->parent = this;
        parent->get_child<dir>() = this;
    }

    void rotate(Direction dir)
    {
        if (dir == Direction::Left)
            rotate<Direction::Left>();
        else
            rotate<Direction::Right>();
    }

    template<Direction dir>
    inline const RedBlackTree *&get_child() const
    {
        if constexpr (dir == Direction::Left)
            return left;
        else
            return right;
    }

    template<Direction dir>
    inline RedBlackTree *&get_child()
    {
        if constexpr (dir == Direction::Left)
            return left;
        else
            return right;
    }

    inline RedBlackTree *&get_child(Direction dir)
    {
        if (dir == Direction::Left)
            return left;
        else
            return right;
    }

    inline Direction get_dir() const
    {
        return is_left() ? Direction::Left : Direction::Right;
    }

    void balance()
    {
        RedBlackTree *node = this;
        while ((node = node->balance_step()));
    }

    RedBlackTree *balance_step()
    {
        if (is_root()) {
            color = Color::Black;
            return nullptr;
        }
        if (parent->is_black())
            return nullptr;

        const Direction parent_dir = parent->get_dir();
        RedBlackTree *uncle = parent->parent->get_child(opposite(parent_dir));
        if (uncle && uncle->is_red()) {
            uncle->color = Color::Black;
            parent->color = Color::Black;
            parent->parent->color = Color::Red;
            return parent->parent;
        } else if (parent_dir != get_dir()) {
            RedBlackTree *former_parent = parent;
            parent->rotate(opposite(get_dir()));
            return former_parent;
        } else {
            parent->color = Color::Black;
            parent->parent->color = Color::Red;
            parent->parent->rotate(opposite(get_dir()));
            return nullptr;
        }
    }

public:
    T value;
private:
    Color color = Color::Black;
    RedBlackTree *left = nullptr, *right = nullptr, *parent = nullptr;
};

