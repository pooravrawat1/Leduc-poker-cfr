#include <cassert>
#include <cstdint>
#include <iostream>

#include "leduc/game_state.h"

using namespace leduc;

namespace
{

void test_initial_state_actions()
{
    const GameState state = GameState::make_initial(CARD_J, CARD_Q);

    assert(!state.is_terminal());
    assert(!state.is_chance_node());
    assert(state.legal_actions_mask() == 0x06u);
    assert(state.count_legal_actions() == 2u);
}

void test_facing_bet_actions()
{
    GameState state = GameState::make_initial(CARD_J, CARD_Q);
    state.to_call = BET_SIZE_ROUND0;
    state.raises_this_round = 1;

    assert(state.legal_actions_mask() == 0x07u);
    assert(state.count_legal_actions() == 3u);
}

void test_capped_raise_actions()
{
    GameState state = GameState::make_initial(CARD_J, CARD_Q);
    state.to_call = BET_SIZE_ROUND0;
    state.raises_this_round = MAX_RAISES_PER_ROUND;

    assert(state.legal_actions_mask() == 0x03u);
    assert(state.count_legal_actions() == 2u);
}

void test_non_decision_nodes_have_no_actions()
{
    GameState folded = GameState::make_initial(CARD_J, CARD_Q);
    folded.folded = 0;
    assert(folded.is_terminal());
    assert(folded.legal_actions_mask() == 0x00u);
    assert(folded.count_legal_actions() == 0u);

    GameState chance = GameState::make_initial(CARD_J, CARD_Q);
    chance.round = 1;
    chance.cards[2] = CARD_NONE;
    assert(chance.is_chance_node());
    assert(chance.legal_actions_mask() == 0x00u);
    assert(chance.count_legal_actions() == 0u);

    GameState showdown = GameState::make_initial(CARD_J, CARD_Q);
    showdown.round = NUM_ROUNDS;
    assert(showdown.is_terminal());
    assert(showdown.legal_actions_mask() == 0x00u);
    assert(showdown.count_legal_actions() == 0u);
}

} // namespace

int main()
{
    test_initial_state_actions();
    test_facing_bet_actions();
    test_capped_raise_actions();
    test_non_decision_nodes_have_no_actions();

    std::cout << "All tests passed for game-state operations\n";
    return 0;
}
