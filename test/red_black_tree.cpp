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

    void create_random_rb_tree(int nr_nodes);

    void cleanup_nodes();

    void test_rb_tree_properties();
    void _test_rb_tree_properties(RedBlackTree<int> *root, int& nr_black_nodes, int depth = 0);

    static void print_rbtree(RedBlackTree<int> *root, int depth = 0);

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

void RedBlackTreeTest::create_random_rb_tree(int nr_nodes)
{
    if (nr_nodes == 0) {
        root = nullptr;
        return;
    }

    root = create_node(rand());

    while (--nr_nodes) {
        RedBlackTree<int> *random_leave = root;
        while (random_leave->get_left() && random_leave->get_right())
            random_leave = rand() % 2 ? random_leave->get_left() : random_leave->get_right();
        RedBlackTree<int> *node = create_node(rand());

        if (random_leave->get_left() && rand() % 2)
            random_leave->insert_right(node);
        else
            random_leave->insert_left(node);
        root = root->get_root();
    }
}

void RedBlackTreeTest::cleanup_nodes()
{
    nodes_container.clear();
    root = nullptr;
}

void RedBlackTreeTest::test_rb_tree_properties()
{
    int nr_black_nodes = 0;
    _test_rb_tree_properties(root, nr_black_nodes);
}

void RedBlackTreeTest::_test_rb_tree_properties(RedBlackTree<int> *root, int& nr_black_nodes, int depth)
{
    if (!root) {
        nr_black_nodes = 1;
        return;
    }

    ASSERT_FALSE(root->is_red() && root->is_root()) << "Depth: " << depth;
    ASSERT_FALSE(root->is_red() && root->get_parent()->is_red()) << "Depth: " << depth
        << " Value: " << root->value << " Parent value: " << root->get_parent()->value;

    int left_nr_black_nodes = 0;
    _test_rb_tree_properties(root->get_left(), left_nr_black_nodes, depth + 1);
    if (!left_nr_black_nodes)
        return;
    int right_nr_black_nodes = 0;
    _test_rb_tree_properties(root->get_right(), right_nr_black_nodes, depth + 1);
    if (!right_nr_black_nodes)
        return;

    ASSERT_EQ(left_nr_black_nodes, right_nr_black_nodes) << "Depth: " << depth;

    nr_black_nodes = root->is_black() + left_nr_black_nodes;
}

void RedBlackTreeTest::print_rbtree(RedBlackTree<int> *root, int depth)
{
    if (!root) {
        std::cout << std::string(depth, '\t') << "(nil)" << std::endl;
        return;
    }

    print_rbtree(root->get_right(), depth + 1);
    std::cout << std::string(depth, '\t') << (root->is_black() ? "\033[37mB" : "\033[31mR") + std::to_string(depth) + "\033[0m" << std::endl;
    print_rbtree(root->get_left(), depth + 1);
}


TEST_F(RedBlackTreeTest, InitialStateInsertion)
{
    create_initial_state();
    auto bt_root = create_initial_state_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, RedUncleCaseInsertion)
{
    create_red_uncle_case();
    auto bt_root = create_red_uncle_case_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, LineCaseInsertion)
{
    create_line_case();
    auto bt_root = create_line_case_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, TriangleCaseInsertion)
{
    create_triangle_case();
    auto bt_root = create_triangle_case_expected_tree();
    compare_trees(root, bt_root.get());
}

TEST_F(RedBlackTreeTest, PreservesRedBlackTreeProperies)
{
    create_random_rb_tree(rand() % 100'000 + 100'000);
    test_rb_tree_properties();
}

TEST_F(RedBlackTreeTest, RedBlackCaseRemoval)
{
    create_triangle_case();
    root = root->get_root();
    print_rbtree(root);
    root->get_right()->remove();
    root = root->get_root();
    std::cout << "--------------------------\n";
    print_rbtree(root);
    test_rb_tree_properties();
}
