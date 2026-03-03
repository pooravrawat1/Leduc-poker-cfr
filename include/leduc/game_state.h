#pragma once

#include <bits/stdc++.h>
#include "types.h"

namespace leduc
{

    // ---------------------------------------------------------------------------
    // GameState – compact 16-byte game state representation
    // ---------------------------------------------------------------------------
    // All fields are uint8_t to minimise memory footprint and maximise cache
    // efficiency.  The struct fits in exactly one 16-byte aligned slot, so two
    // states occupy a single 32-byte cache line.
    //
    // Field overview:
    //   cards[0]          – Player 0's private card  (CARD_J/Q/K)
    //   cards[1]          – Player 1's private card  (CARD_J/Q/K)
    //   cards[2]          – Public card              (CARD_J/Q/K or CARD_NONE)
    //   round             – Current betting round    (0 = pre-flop, 1 = flop)
    //   player            – Player to act next       (0 or 1)
    //   pot               – Total chips in the pot
    //   to_call           – Chips the current player must put in to call
    //   raises_this_round – Number of raises so far in the current round (0–2)
    //   action_history    – Bit-packed sequence of actions taken this hand
    //   folded            – NO_FOLD (0xFF) if nobody has folded; otherwise the
    //                       index of the player who folded (0 or 1)
    //   padding[6]        – Explicit padding to reach exactly 16 bytes

    struct alignas(16) GameState
    {
        uint8_t cards[3];          // [p0_card, p1_card, public_card]
        uint8_t round;             // 0 or 1
        uint8_t player;            // 0 or 1 (player to act)
        uint8_t pot;               // Current pot size (chips)
        uint8_t to_call;           // Chips required to call (0 = free check)
        uint8_t raises_this_round; // Raises issued in the current round (0–2)
        uint8_t action_history;    // Bit-packed action sequence (2 bits per action)
        uint8_t folded;            // NO_FOLD (0xFF) or folding player index
        uint8_t padding[6];        // Pad to 16 bytes total

        // -----------------------------------------------------------------------
        // Factory: build the initial state for a new hand.
        // Cards must be provided by the caller (dealt externally).
        // -----------------------------------------------------------------------
        static GameState make_initial(uint8_t p0_card, uint8_t p1_card) noexcept
        {
            GameState s{};
            s.cards[0] = p0_card;
            s.cards[1] = p1_card;
            s.cards[2] = CARD_NONE; // no public card yet
            s.round = 0;
            s.player = 0;        // player 0 acts first pre-flop
            s.pot = INITIAL_POT; // both antes already in
            s.to_call = 0;       // no outstanding bet
            s.raises_this_round = 0;
            s.action_history = 0;
            s.folded = NO_FOLD;
            return s;
        }

        // -----------------------------------------------------------------------
        // Convenience predicates
        // -----------------------------------------------------------------------

        /// True once the hand is over (fold or showdown).
        bool is_terminal() const noexcept
        {
            // Fold: someone folded
            if (folded != NO_FOLD)
                return true;
            // Showdown: round 1 has ended – both players checked or call ended it.
            // Encoded as round == 1 AND to_call == 0 AND action_history shows
            // at least one action was taken this round (the last actor called/checked).
            // The game engine sets round = 2 after round 1 ends; check for that.
            return (round >= NUM_ROUNDS);
        }

        /// True when the public card has not yet been revealed (chance node).
        bool is_chance_node() const noexcept
        {
            return (round == 1) && (cards[2] == CARD_NONE);
        }
    };

    // Verify the struct is exactly 16 bytes at compile time.
    static_assert(sizeof(GameState) == 16, "GameState must be exactly 16 bytes");
    static_assert(alignof(GameState) == 16, "GameState must be 16-byte aligned");

} // namespace leduc
