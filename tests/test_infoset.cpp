#include <iostream>
#include <cassert>
#include "leduc/infoset.h"
#include "leduc/game_state.h"

using namespace leduc;

void test_initial_infoset() {
    GameState state = GameState::make_initial(CARD_J, CARD_Q);
    uint16_t key = compute_infoset_key(state, 0); // Player 0 to act
    
    uint8_t round, player_card, public_card, history, action_seq;
    decode_infoset_key(key, round, player_card, public_card, history, action_seq);
    
    assert(round == 0);
    assert(player_card == CARD_J);
    assert(public_card == CARD_NONE);
    assert(history == 0);
    assert(action_seq == 0); // (empty)
    
    std::cout << "test_initial_infoset passed\n";
}

void test_round1_infoset() {
    GameState state = GameState::make_initial(CARD_K, CARD_J);
    state.round = 1;
    state.cards[2] = CARD_Q; // Public card is Queen
    state.history_r0 = 0x05; // Round 0 was Check-Check
    state.action_history = 0x02; // Current round: player 0 Bet
    
    uint16_t key = compute_infoset_key(state, 1); // Player 1 to act
    
    uint8_t round, player_card, public_card, history, action_seq;
    decode_infoset_key(key, round, player_card, public_card, history, action_seq);
    
    assert(round == 1);
    assert(player_card == CARD_J);
    assert(public_card == CARD_Q);
    // history bits [0..2] = encode_terminal_seq(0x05) = 1
    // history bits [3..5] = 0
    // history value = 1
    assert(history == 1);
    assert(action_seq == 2); // Bet
    
    std::cout << "test_round1_infoset passed\n";
}

int main() {
    test_initial_infoset();
    test_round1_infoset();
    std::cout << "All tests passed for information set encoding\n";
    return 0;
}
