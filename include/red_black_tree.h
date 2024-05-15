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

    template<typename ...Args>
    RedBlackTree(Args&&... args) : value(std::forward<Args>(args)...)
    {}

    void insert_left(RedBlackTree *node)
    {
        insert_common(node);
        left = node;
        node->insert_fixup();
    }

    void insert_right(RedBlackTree *node)
    {
        insert_common(node);
        right = node;
        node->insert_fixup();
    }

    void remove()
    {
        if (left && right) {
            RedBlackTree *inorder_next = right;
            while (inorder_next->left)
                inorder_next = inorder_next->left; // find the in-order successor of the node

            // pretend we are deleting the in-order successor node by swapping it with this node
            // (by pretending we're just swapping the values because type T isn't guaranteed to be swappable)
            if (inorder_next == right) {
                transplant(inorder_next);
                parent = inorder_next;
                right = parent->right;
                parent->right = this;
            } else {
                std::swap(inorder_next->parent, parent);
                parent->left = this;
                left->parent = inorder_next;
                right->parent = inorder_next;
                std::swap(inorder_next->right, right);
                if (right)
                    right->parent = this;
            }
            inorder_next->left = left; left = nullptr;
            std::swap(inorder_next->color, color);
        }

        RedBlackTree *replacer_node = nullptr;
        if (left) {
            transplant(left);
            replacer_node = left;
            left = nullptr;
        } else if (right) {
            transplant(right);
            replacer_node = right;
            right = nullptr;
        } else {
            replacer_node = this; // treating the deleted node as a temporary NIL node
        }

        if (color == Color::Black)
            replacer_node->remove_fixup();

        // either the deleted node is red and the replacer node is already black
        // or the deleted node is black and the replacer must have the same color
        replacer_node->color = Color::Black;
        // reset deleted node to the state it was before being inserted
        color = Color::Black;
        if (this == replacer_node && !is_root())
            parent->get_child(get_dir()) = nullptr;
        parent = nullptr;
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

    void insert_common(RedBlackTree *node)
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

    void insert_fixup()
    {
        RedBlackTree *node = this;
        while ((node = node->insert_fixup_step()));
    }

    RedBlackTree *insert_fixup_step()
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

    inline void transplant(RedBlackTree *replacer) const
    {
        replacer->parent = parent;
        if (!is_root())
            parent->get_child(get_dir()) = replacer;
    }

    void remove_fixup()
    {
        RedBlackTree *node = this;
        while ((node = node->remove_fixup_step()));
    }

    RedBlackTree *remove_fixup_step()
    {
        if (is_root() || is_red()) {
            color = Color::Black;
            return nullptr;
        }

        Direction dir = get_dir();
        Direction sibling_dir = opposite(dir);
        RedBlackTree *sibling = parent->get_child(sibling_dir);
        if (sibling->is_red()) {
            sibling->color = Color::Black;
            parent->color = Color::Red;
            parent->rotate(dir);
            return this;
        } else if ((!sibling->left || sibling->left->is_black())
                && (!sibling->right || sibling->right->is_black())) {
            sibling->color = Color::Red;
            return parent;
        } else if (!sibling->get_child(sibling_dir)
                || sibling->get_child(sibling_dir)->is_black()) {
            sibling->get_child(dir)->color = Color::Black;
            sibling->color = Color::Red;
            sibling->rotate(sibling_dir);
            return this;
        } else {
            sibling->color = parent->color;
            parent->color = Color::Black;
            sibling->get_child(sibling_dir)->color = Color::Black;
            parent->rotate(dir);
            if (parent->parent->is_root())
                return parent->parent;
            else
                return nullptr;
        }
    }

public:
    T value;
private:
    Color color = Color::Black;
    RedBlackTree *left = nullptr, *right = nullptr, *parent = nullptr;
};

