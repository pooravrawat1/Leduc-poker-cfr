#pragma once

#include <bits/stdc++.h>

namespace leduc
{

    // ---------------------------------------------------------------------------
    // Action enum
    // ---------------------------------------------------------------------------

    /// The three possible actions in Leduc Poker.
    /// CALL doubles as CHECK when to_call == 0; RAISE doubles as BET.
    enum class Action : uint8_t
    {
        FOLD = 0,
        CALL = 1, ///< Also CHECK when to_call == 0
        RAISE = 2 ///< Also BET  when to_call == 0
    };

    /// Total number of possible actions.
    constexpr int NUM_ACTIONS = 3;

    // ---------------------------------------------------------------------------
    // Card encoding constants
    // ---------------------------------------------------------------------------

    /// Private/public card ranks (integer encoding).
    constexpr uint8_t CARD_J = 0;    ///< Jack  – weakest
    constexpr uint8_t CARD_Q = 1;    ///< Queen
    constexpr uint8_t CARD_K = 2;    ///< King  – strongest
    constexpr uint8_t CARD_NONE = 3; ///< Sentinel: no card dealt yet

    /// Deck composition.
    constexpr int NUM_RANKS = 3;                          ///< J, Q, K
    constexpr int CARDS_PER_RANK = 2;                     ///< Two of each rank
    constexpr int DECK_SIZE = NUM_RANKS * CARDS_PER_RANK; ///< 6 cards total

    // ---------------------------------------------------------------------------
    // Bet-size constants (chips)
    // ---------------------------------------------------------------------------

    /// Fixed-limit bet / raise size for each round.
    constexpr uint8_t BET_SIZE_ROUND0 = 2; ///< Pre-flop (round 0)
    constexpr uint8_t BET_SIZE_ROUND1 = 4; ///< Flop     (round 1)

    // ---------------------------------------------------------------------------
    // Game-structure constants
    // ---------------------------------------------------------------------------

    /// Number of players.
    constexpr int NUM_PLAYERS = 2;

    /// Number of betting rounds.
    constexpr int NUM_ROUNDS = 2;

    /// Ante paid by each player before the hand begins.
    constexpr uint8_t ANTE = 1;

    /// Initial pot size (both players post their ante).
    constexpr uint8_t INITIAL_POT = NUM_PLAYERS * ANTE; ///< 2 chips

    /// Maximum raises allowed **per round** (initial bet counts as raise 1).
    /// Round 0: check → bet → raise  (2 raises max)
    /// Round 1: same structure
    constexpr uint8_t MAX_RAISES_PER_ROUND = 2;

    // ---------------------------------------------------------------------------
    // Information-set / storage constants
    // ---------------------------------------------------------------------------

    /// Upper bound on the number of distinct information sets in Leduc Poker.
    /// The game has ~936 reachable info sets; 1024 gives a power-of-two budget
    /// that comfortably covers all of them with direct array indexing.
    constexpr size_t MAX_INFOSETS = 1024;

    /// Sentinel value used to indicate "no player has folded".
    constexpr uint8_t NO_FOLD = 0xFF;

} // namespace leduc
