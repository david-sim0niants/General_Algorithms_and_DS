#pragma once

#include <utility>
#include <type_traits>

/* Red-Black Tree data structure, responsible for keeping the binary search tree balanced,
 * providing the insert_left(), insert_right(), and remove() operations.
 * It's not responsible for searching for the elements, finding the correct place to insert them,
 * or finding the matching node to remove them.
 * It's also not responsible for allocating node data or managing nodes' memory.
 *
 * To insert a new node under an existing node, first create the new node,
 * call insert_left() or insert_right() on the existing node (depending on where you decide to put it),
 * and pass the pointer to the newly created node. As after insertion and re-balancing the root may have changed,
 * call get_root() on the root node to find the new root of the tree.
 * Make sure the new node stays alive as long as it has not been removed from the tree.
 * The tree is not responsible for managing node's memory.
 *
 * To remove a node, simply call remove() on the existing node and get_root() on the root to find the new root.
 * Calling remove() on a root node that has children may result in losing access to the rest of the tree.
 * In that case keep one of root node's children temporarily and after removing call get_root() on it
 * to traverse back to the new root. Note, if the root has no children, you're removing the last node of the tree.
 * */
template<typename T>
class RedBlackTree {
private:
    /* Color enum. */
    enum class Color : char {
        Black = 0, Red = 1
    };

    /* This enum is used to specify if the node is a left or right child of its parent,
     * and for specifying in which direction to rotate the node. */
    enum class Direction {
        Left = 0, Right = 1
    };

public:
    /* Default constructor. Only possible if the value type is default-constructible. */
    RedBlackTree() = default;

    RedBlackTree(T&& value) : value(std::move(value))
    {}

    RedBlackTree(const T& value) : value(value)
    {}

    /* Emplace constructor. Takes arguments needed to construct the value. */
    template<typename ...Args>
    RedBlackTree(Args&&... args) : value(std::forward<Args>(args)...)
    {}

    /* Insert node to the left of this node. */
    void insert_left(RedBlackTree *node)
    {
        insert_common(node);
        left = node;
        node->insert_fixup();
    }

    /* Insert node to the right of this node. */
    void insert_right(RedBlackTree *node)
    {
        insert_common(node);
        right = node;
        node->insert_fixup();
    }

    /* Remove 'this' node. */
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

    /* Find the root of the tree. */
    inline RedBlackTree *get_root()
    {
        RedBlackTree *node = this;
        while (!node->is_root())
            node = node->get_parent();
        return node;
    }

private:
    /* Find the opposite direction. */
    inline static constexpr Direction opposite(Direction dir)
    {
        return static_cast<Direction>( ! static_cast<std::underlying_type_t<Direction>>(dir));
    }

    /* Initialize the new node to be red and its parent to be 'this'. */
    void insert_common(RedBlackTree *node)
    {
        node->parent = this;
        node->color = Color::Red;
    }

    /* Rotate this node left or right (compile-time). */
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

    /* Rotate this node left or right (run-time). */
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

    /* Post-insert balancing operation. */
    void insert_fixup()
    {
        RedBlackTree *node = this;
        while ((node = node->insert_fixup_step()));
    }

    /* Perform one step of insertion fix-up.
     * Return the next node to operate on, or null if nothing else should be done. */
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
        if (uncle && uncle->is_red()) { // red uncle case
            uncle->color = Color::Black;
            parent->color = Color::Black;
            parent->parent->color = Color::Red;
            return parent->parent;
        } else if (parent_dir != get_dir()) { // black uncle case with a triangle formed
            RedBlackTree *former_parent = parent;
            parent->rotate(opposite(get_dir()));
            return former_parent;
        } else { // black uncle case with a line formed
            parent->color = Color::Black;
            parent->parent->color = Color::Red;
            parent->parent->rotate(opposite(get_dir()));
            return nullptr;
        }
    }

    /* Makes the replacer node parent to be 'this' node's parent. */
    inline void transplant(RedBlackTree *replacer) const
    {
        replacer->parent = parent;
        if (!is_root())
            parent->get_child(get_dir()) = replacer;
    }

    /* Post-remove balancing operation. */
    void remove_fixup()
    {
        RedBlackTree *node = this;
        while ((node = node->remove_fixup_step()));
    }

    /* Perform one step of removal fix-up.
     * Return the next node to operate on, or null if nothing else should be done. */
    RedBlackTree *remove_fixup_step()
    {
        if (is_root() || is_red()) {
            color = Color::Black;
            return nullptr;
        }

        Direction dir = get_dir();
        Direction sibling_dir = opposite(dir);
        RedBlackTree *sibling = parent->get_child(sibling_dir);
        if (sibling->is_red()) { // Red sibling case
            sibling->color = Color::Black;
            parent->color = Color::Red;
            parent->rotate(dir);
            return this;
        } else if ((!sibling->left || sibling->left->is_black())
                && (!sibling->right || sibling->right->is_black())) {
            // Black sibling with both children black case
            sibling->color = Color::Red;
            return parent;
        } else if (!sibling->get_child(sibling_dir)
                || sibling->get_child(sibling_dir)->is_black()) {
            // A case when the black sibling's child, the direction of which matches the direction of the sibling, is black.
            sibling->get_child(dir)->color = Color::Black;
            sibling->color = Color::Red;
            sibling->rotate(sibling_dir);
            return this;
        } else {
            // A case when the black sibling's child, the direction of which matches the direction of the sibling, is red.
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
    T value; /* Value is exposed publicly as the red-black tree doesn't even use it. */
private:
    Color color = Color::Black;
    RedBlackTree *left = nullptr, *right = nullptr, *parent = nullptr;
};

