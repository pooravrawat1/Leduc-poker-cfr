#pragma once

#include <bits/stdc++.h>
using namespace std;

namespace leduc
{

    // Game constants
    constexpr array<char, 3> RANKS = {'J', 'Q', 'K'};
    constexpr array<int, 2> BET_SIZES = {2, 4}; // Bet sizes by round (0, 1)
    constexpr int MAX_RAISES_PER_ROUND = 1;
    constexpr int DECK_SIZE = 6; // 2 of each rank

    /// Represents a single state in Leduc Poker.
    /// A state is immutable; apply_action() returns a new state.
    struct LeducState
    {
        pair<char, char> private_cards; // (player 0's card, player 1's card)
        optional<char> public_card;     // null in round 0
        vector<vector<string>> history; // per-round action sequences
        int current_round;              // 0 or 1
        int current_player;             // 0 or 1
        int pot;
        int chips_to_call;
        int raises_this_round;
        optional<int> folded; // which player folded, if any

        /// Check if this is a terminal state (game over).
        bool is_terminal() const;

        /// Check if this is a chance node (before card dealing/revealing).
        bool is_chance_node() const;

        /// Return list of legal actions in this state.
        vector<string> legal_actions() const;

        /// Apply action and return new state (immutable).
        LeducState apply_action(const string &action) const;

        /// Return utility for player (only valid on terminal states).
        /// In Leduc, payoff is in chips won.
        double utility(int player) const;

        /// Return information set key for perfect recall and imperfect info.
        /// Format: "{private_card}|{public_card_or_?}|{round0_hist}/{round1_hist}"
        string info_set_key(int player) const;
    };

    /// Leduc Poker game engine.
    class LeducGame
    {
    public:
        /// Return root game state (before card dealing).
        LeducState initial_state() const;

        /// Sample chance outcomes (card deals) with probabilities.
        /// Returns list of (card_action, probability) pairs.
        vector<pair<string, double>> sample_chance(const LeducState &state) const;

        /// Enumerate all possible initial hands (for testing/exploitability).
        vector<LeducState> get_all_hands() const;
    };

} // namespace leduc
