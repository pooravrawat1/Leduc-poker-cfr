#include <cassert>
#include <cmath>
#include <iostream>

#include "leduc/cfr_node.h"

using namespace leduc;

namespace
{

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

CFRNode make_three_action_node()
{
    CFRNode node;
    node.reset();
    node.num_actions = 3;
    node.action_mask = 0x07u;
    return node;
}

void test_positive_regrets_are_normalized()
{
    CFRNode node = make_three_action_node();

    node.regret_sum[0] = 2.0f;
    node.regret_sum[1] = 6.0f;
    node.regret_sum[2] = 2.0f;

    node.update_strategy();

    assert(near(node.strategy[0], 0.2f));
    assert(near(node.strategy[1], 0.6f));
    assert(near(node.strategy[2], 0.2f));
}

void test_negative_regrets_are_clamped_to_zero()
{
    CFRNode node = make_three_action_node();

    node.regret_sum[0] = -5.0f;
    node.regret_sum[1] = 3.0f;
    node.regret_sum[2] = 1.0f;

    node.update_strategy();

    assert(near(node.strategy[0], 0.0f));
    assert(near(node.strategy[1], 0.75f));
    assert(near(node.strategy[2], 0.25f));
}

void test_all_non_positive_regrets_use_uniform_strategy()
{
    CFRNode node = make_three_action_node();

    node.regret_sum[0] = -2.0f;
    node.regret_sum[1] = 0.0f;
    node.regret_sum[2] = -5.0f;

    node.update_strategy();

    assert(near(node.strategy[0], 1.0f / 3.0f));
    assert(near(node.strategy[1], 1.0f / 3.0f));
    assert(near(node.strategy[2], 1.0f / 3.0f));
}

void test_illegal_actions_get_zero_probability()
{
    CFRNode node;
    node.reset();
    node.num_actions = 2;
    node.action_mask = 0x06u;

    node.regret_sum[0] = 100.0f;
    node.regret_sum[1] = 3.0f;
    node.regret_sum[2] = 1.0f;

    node.update_strategy();

    assert(near(node.strategy[0], 0.0f));
    assert(near(node.strategy[1], 0.75f));
    assert(near(node.strategy[2], 0.25f));
}

void test_average_strategy_normalizes_strategy_sum()
{
    CFRNode node = make_three_action_node();

    node.strategy_sum[0] = 2.0f;
    node.strategy_sum[1] = 6.0f;
    node.strategy_sum[2] = 2.0f;

    float average[NUM_ACTIONS];
    node.get_average_strategy(average);

    assert(near(average[0], 0.2f));
    assert(near(average[1], 0.6f));
    assert(near(average[2], 0.2f));
}

void test_average_strategy_ignores_illegal_actions()
{
    CFRNode node;
    node.reset();
    node.num_actions = 2;
    node.action_mask = 0x06u;

    node.strategy_sum[0] = 100.0f;
    node.strategy_sum[1] = 3.0f;
    node.strategy_sum[2] = 1.0f;

    float average[NUM_ACTIONS];
    node.get_average_strategy(average);

    assert(near(average[0], 0.0f));
    assert(near(average[1], 0.75f));
    assert(near(average[2], 0.25f));
}

void test_empty_average_strategy_uses_uniform()
{
    CFRNode node = make_three_action_node();

    float average[NUM_ACTIONS];
    node.get_average_strategy(average);

    assert(near(average[0], 1.0f / 3.0f));
    assert(near(average[1], 1.0f / 3.0f));
    assert(near(average[2], 1.0f / 3.0f));
}

void test_empty_node_returns_zero_strategy()
{
    CFRNode node;
    node.reset();

    float average[NUM_ACTIONS];
    node.update_strategy();
    node.get_average_strategy(average);

    assert(near(node.strategy[0], 0.0f));
    assert(near(node.strategy[1], 0.0f));
    assert(near(node.strategy[2], 0.0f));
    assert(near(average[0], 0.0f));
    assert(near(average[1], 0.0f));
    assert(near(average[2], 0.0f));
}

} // namespace

int main()
{
    test_positive_regrets_are_normalized();
    test_negative_regrets_are_clamped_to_zero();
    test_all_non_positive_regrets_use_uniform_strategy();
    test_illegal_actions_get_zero_probability();
    test_average_strategy_normalizes_strategy_sum();
    test_average_strategy_ignores_illegal_actions();
    test_empty_average_strategy_uses_uniform();
    test_empty_node_returns_zero_strategy();

    std::cout << "All tests passed for CFRNode regret matching\n";
    return 0;
}
