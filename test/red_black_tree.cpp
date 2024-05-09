#include <gtest/gtest.h>

#include "red_black_tree.h"
#include <unordered_set>
#include <utility>


class RedBlackTreeTest : public ::testing::Test {
protected:
    enum Color { Black, Red };

    struct BinaryTree {
        int value;
        Color color;
        std::unique_ptr<BinaryTree> left, right;

        BinaryTree(int value, Color color, std::unique_ptr<BinaryTree>&& left = nullptr, std::unique_ptr<BinaryTree>&& right = nullptr)
            : value(value), color(color), left(std::move(left)), right(std::move(right))
        {}
    };

    RedBlackTree<int> *create_node(int value);
    void delete_node(RedBlackTree<int> *node);

    template<typename ...Args>
    inline static std::unique_ptr<BinaryTree> make_bt(Args&&... args)
    {
        return std::make_unique<BinaryTree>(std::forward<Args>(args)...);
    }

    static void compare_trees(RedBlackTree<int> *rb_root, BinaryTree *bt_root);

    void create_initial_state();
    void create_red_uncle_case();
    void create_line_case();
    void create_triangle_case();

    static std::unique_ptr<BinaryTree> create_initial_state_expected_tree();
    static std::unique_ptr<BinaryTree> create_red_uncle_case_expected_tree();
    static std::unique_ptr<BinaryTree> create_line_case_expected_tree();
    static std::unique_ptr<BinaryTree> create_triangle_case_expected_tree();

    RedBlackTree<int> *root = nullptr;
    std::unordered_map<RedBlackTree<int> *, std::unique_ptr<RedBlackTree<int>>> nodes_container;
};

RedBlackTree<int> *RedBlackTreeTest::create_node(int value)
{
    std::unique_ptr<RedBlackTree<int>> node = std::make_unique<RedBlackTree<int>>(value);
    RedBlackTree<int> *node_raw = node.get();
    nodes_container.emplace(node_raw, std::move(node));
    return node_raw;
}

void RedBlackTreeTest::delete_node(RedBlackTree<int> *node)
{
    nodes_container.erase(node);
}

void RedBlackTreeTest::compare_trees(RedBlackTree<int> *rb_root, BinaryTree *bt_root)
{
    EXPECT_EQ(rb_root != nullptr, bt_root != nullptr);
    if (!rb_root)
        return;
    EXPECT_EQ(rb_root->value, bt_root->value);
    if (bt_root->color == Black)
        EXPECT_TRUE(rb_root->is_black());
    else
        EXPECT_TRUE(rb_root->is_red());
    compare_trees(rb_root->get_left(), bt_root->left.get());
    compare_trees(rb_root->get_right(), bt_root->right.get());
}

void RedBlackTreeTest::create_initial_state()
{
    root = create_node(0);
    root->insert_right(create_node(1));
    root->get_right()->insert_right(create_node(2));
    root = root->get_root();
}

void RedBlackTreeTest::create_red_uncle_case()
{
    create_initial_state();
    root->get_right()->insert_right(create_node(3));
    root = root->get_root();
}

void RedBlackTreeTest::create_line_case()
{
    create_red_uncle_case();
    root->get_right()->get_right()->insert_right(create_node(4));
    root = root->get_root();
}

void RedBlackTreeTest::create_triangle_case()
{
    create_line_case();
    root->get_left()->insert_left(create_node(-2));
    root->get_left()->get_left()->insert_right(create_node(-1));
    root = root->get_root();
}

std::unique_ptr<RedBlackTreeTest::BinaryTree> RedBlackTreeTest::create_initial_state_expected_tree()
{
    return
        make_bt(1, Black,
            make_bt(0, Red),
            make_bt(2, Red)
        );
}

std::unique_ptr<RedBlackTreeTest::BinaryTree> RedBlackTreeTest::create_red_uncle_case_expected_tree()
{
    return
        make_bt(1, Black,
            make_bt(0, Black),
            make_bt(2, Black,
                nullptr,
                make_bt(3, Red)));
}

std::unique_ptr<RedBlackTreeTest::BinaryTree> RedBlackTreeTest::create_line_case_expected_tree()
{
    return
        make_bt(1, Black,
            make_bt(0, Black),
            make_bt(3, Black,
                make_bt(2, Red),
                make_bt(4, Red)));
}

std::unique_ptr<RedBlackTreeTest::BinaryTree> RedBlackTreeTest::create_triangle_case_expected_tree()
{
    return
        make_bt(1, Black,
            make_bt(-1, Black,
                make_bt(-2, Red),
                make_bt(0, Red)),
            make_bt(3, Black,
                make_bt(2, Red),
                make_bt(4, Red)));
}


TEST_F(RedBlackTreeTest, InitialState)
{
    create_initial_state();
    auto bt_root = create_initial_state_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, RedUncleCase)
{
    create_red_uncle_case();
    auto bt_root = create_red_uncle_case_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, LineCase)
{
    create_line_case();
    auto bt_root = create_line_case_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, TriangleCase)
{
    create_triangle_case();
    auto bt_root = create_triangle_case_expected_tree();
    compare_trees(root, bt_root.get());
}

