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

    assert(state.committed[0] == ANTE);
    assert(state.committed[1] == ANTE);
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

void test_first_check_switches_player()
{
    const GameState state = GameState::make_initial(CARD_J, CARD_Q);
    const GameState next = state.apply_action(Action::CALL);

    assert(next.round == 0);
    assert(next.player == 1u);
    assert(next.pot == INITIAL_POT);
    assert(next.to_call == 0u);
    assert(next.action_history == 0x01u);
    assert(!next.is_terminal());
}

void test_check_check_reaches_chance_node()
{
    const GameState p1_to_act = GameState::make_initial(CARD_J, CARD_Q).apply_action(Action::CALL);
    const GameState chance = p1_to_act.apply_action(Action::CALL);

    assert(chance.round == 1u);
    assert(chance.player == 0u);
    assert(chance.cards[2] == CARD_NONE);
    assert(chance.history_r0 == 0x05u);
    assert(chance.action_history == 0u);
    assert(chance.is_chance_node());
}

void test_bet_and_call_reaches_chance_node()
{
    const GameState bet = GameState::make_initial(CARD_J, CARD_Q).apply_action(Action::RAISE);

    assert(bet.round == 0u);
    assert(bet.player == 1u);
    assert(bet.pot == INITIAL_POT + BET_SIZE_ROUND0);
    assert(bet.committed[0] == ANTE + BET_SIZE_ROUND0);
    assert(bet.committed[1] == ANTE);
    assert(bet.to_call == BET_SIZE_ROUND0);
    assert(bet.raises_this_round == 1u);
    assert(bet.action_history == 0x02u);

    const GameState chance = bet.apply_action(Action::CALL);

    assert(chance.round == 1u);
    assert(chance.pot == INITIAL_POT + BET_SIZE_ROUND0 + BET_SIZE_ROUND0);
    assert(chance.committed[0] == ANTE + BET_SIZE_ROUND0);
    assert(chance.committed[1] == ANTE + BET_SIZE_ROUND0);
    assert(chance.to_call == 0u);
    assert(chance.raises_this_round == 0u);
    assert(chance.history_r0 == 0x09u);
    assert(chance.action_history == 0u);
    assert(chance.is_chance_node());
}

void test_raise_calls_existing_bet_before_raising()
{
    const GameState bet = GameState::make_initial(CARD_J, CARD_Q).apply_action(Action::RAISE);
    const GameState raise = bet.apply_action(Action::RAISE);

    assert(raise.pot == INITIAL_POT + BET_SIZE_ROUND0 + BET_SIZE_ROUND0 + BET_SIZE_ROUND0);
    assert(raise.committed[0] == ANTE + BET_SIZE_ROUND0);
    assert(raise.committed[1] == ANTE + BET_SIZE_ROUND0 + BET_SIZE_ROUND0);
    assert(raise.to_call == BET_SIZE_ROUND0);
    assert(raise.raises_this_round == 2u);
    assert(raise.player == 0u);
    assert(raise.action_history == 0x0Au);
}

void test_apply_chance_reveals_public_card()
{
    const GameState chance = GameState::make_initial(CARD_J, CARD_Q)
                                 .apply_action(Action::CALL)
                                 .apply_action(Action::CALL);
    const GameState flop = chance.apply_chance(CARD_K);

    assert(flop.round == 1u);
    assert(flop.cards[2] == CARD_K);
    assert(flop.player == 0u);
    assert(flop.to_call == 0u);
    assert(flop.raises_this_round == 0u);
    assert(flop.action_history == 0u);
    assert(!flop.is_chance_node());
}

void test_round_one_check_check_is_terminal()
{
    const GameState flop = GameState::make_initial(CARD_J, CARD_Q)
                               .apply_action(Action::CALL)
                               .apply_action(Action::CALL)
                               .apply_chance(CARD_K);

    const GameState terminal = flop.apply_action(Action::CALL).apply_action(Action::CALL);

    assert(terminal.round == NUM_ROUNDS);
    assert(terminal.is_terminal());
    assert(terminal.action_history == 0x05u);
}

void test_showdown_pair_beats_high_card()
{
    assert(GameState::evaluate_showdown(CARD_J, CARD_K, CARD_J) == 1);
    assert(GameState::evaluate_showdown(CARD_K, CARD_J, CARD_J) == -1);
}

void test_showdown_high_card_comparison()
{
    assert(GameState::evaluate_showdown(CARD_K, CARD_Q, CARD_J) == 1);
    assert(GameState::evaluate_showdown(CARD_J, CARD_Q, CARD_K) == -1);
    assert(GameState::evaluate_showdown(CARD_Q, CARD_Q, CARD_K) == 0);
}

void test_fold_terminal_utility()
{
    const GameState folded = GameState::make_initial(CARD_J, CARD_Q)
                                 .apply_action(Action::RAISE)
                                 .apply_action(Action::FOLD);

    assert(folded.is_terminal());
    assert(folded.terminal_utility(0) == 1);
    assert(folded.terminal_utility(1) == -1);
    assert(folded.terminal_utility(0) + folded.terminal_utility(1) == 0);
}

void test_showdown_terminal_utility()
{
    const GameState terminal = GameState::make_initial(CARD_K, CARD_Q)
                                   .apply_action(Action::RAISE)
                                   .apply_action(Action::CALL)
                                   .apply_chance(CARD_K)
                                   .apply_action(Action::CALL)
                                   .apply_action(Action::CALL);

    assert(terminal.is_terminal());
    assert(terminal.terminal_utility(0) == 3);
    assert(terminal.terminal_utility(1) == -3);
    assert(terminal.terminal_utility(0) + terminal.terminal_utility(1) == 0);
}

void test_tie_terminal_utility()
{
    GameState terminal = GameState::make_initial(CARD_Q, CARD_Q);
    terminal.cards[2] = CARD_K;
    terminal.round = NUM_ROUNDS;

    assert(terminal.is_terminal());
    assert(terminal.terminal_utility(0) == 0);
    assert(terminal.terminal_utility(1) == 0);
}

} // namespace

int main()
{
    test_initial_state_actions();
    test_facing_bet_actions();
    test_capped_raise_actions();
    test_non_decision_nodes_have_no_actions();
    test_first_check_switches_player();
    test_check_check_reaches_chance_node();
    test_bet_and_call_reaches_chance_node();
    test_raise_calls_existing_bet_before_raising();
    test_apply_chance_reveals_public_card();
    test_round_one_check_check_is_terminal();
    test_showdown_pair_beats_high_card();
    test_showdown_high_card_comparison();
    test_fold_terminal_utility();
    test_showdown_terminal_utility();
    test_tie_terminal_utility();

    std::cout << "All tests passed for game-state operations\n";
    return 0;
}
