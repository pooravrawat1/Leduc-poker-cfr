#pragma once

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
        uint8_t history_r0;        // Terminal sequence of round 0 (for info-sets)
        uint8_t padding[5];        // Pad to 16 bytes total

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

        /// Bitmask of legal player actions in this state.
        /// bit 0 = FOLD, bit 1 = CALL/CHECK, bit 2 = RAISE/BET.
        uint8_t legal_actions_mask() const noexcept
        {
            const uint8_t active = static_cast<uint8_t>(!is_terminal() && !is_chance_node());
            const uint8_t active_mask = static_cast<uint8_t>(0u - active);

            uint8_t mask = 0;
            mask |= static_cast<uint8_t>(to_call > 0) << static_cast<uint8_t>(Action::FOLD);
            mask |= static_cast<uint8_t>(1u << static_cast<uint8_t>(Action::CALL));
            mask |= static_cast<uint8_t>(raises_this_round < MAX_RAISES_PER_ROUND)
                    << static_cast<uint8_t>(Action::RAISE);

            return static_cast<uint8_t>(mask & active_mask);
        }

        /// Count legal actions in the low three bits of an action mask.
        static uint8_t count_legal_actions(uint8_t action_mask) noexcept
        {
            action_mask &= 0x07u;
            return static_cast<uint8_t>((action_mask & 0x01u) +
                                        ((action_mask >> 1) & 0x01u) +
                                        ((action_mask >> 2) & 0x01u));
        }

        /// Count legal player actions in this state.
        uint8_t count_legal_actions() const noexcept
        {
            return count_legal_actions(legal_actions_mask());
        }

        uint8_t bet_size() const noexcept
        {
            return (round == 0) ? BET_SIZE_ROUND0 : BET_SIZE_ROUND1;
        }

        uint8_t next_action_history(Action action) const noexcept
        {
            return static_cast<uint8_t>(
                (action_history << 2) | static_cast<uint8_t>(action));
        }

        GameState apply_chance(uint8_t public_card) const noexcept
        {
            GameState next = *this;

            next.cards[2] = public_card;
            next.player = 0;
            next.to_call = 0;
            next.raises_this_round = 0;
            next.action_history = 0;

            return next;
        }

        GameState apply_action(Action action) const noexcept
        {
            GameState next = *this;
            const uint8_t new_history = next_action_history(action);

            next.action_history = new_history;

            if (action == Action::FOLD)
            {
                next.folded = player;
                return next;
            }

            if (action == Action::RAISE)
            {
                const uint8_t amount = bet_size();

                next.pot = static_cast<uint8_t>(next.pot + to_call + amount);
                next.to_call = amount;
                next.raises_this_round = static_cast<uint8_t>(raises_this_round + 1);
                next.player = static_cast<uint8_t>(1 - player);

                return next;
            }

            if (action == Action::CALL)
            {
                next.pot = static_cast<uint8_t>(next.pot + to_call);
                next.to_call = 0;

                const bool was_calling_bet = (to_call > 0);
                const bool was_second_check = (to_call == 0 && action_history != 0);
                const bool closes_round = was_calling_bet || was_second_check;

                if (!closes_round)
                {
                    next.player = static_cast<uint8_t>(1 - player);
                    return next;
                }

                if (round == 0)
                {
                    next.history_r0 = new_history;
                    next.round = 1;
                    next.cards[2] = CARD_NONE;
                    next.player = 0;
                    next.raises_this_round = 0;
                    next.action_history = 0;
                    return next;
                }

                next.round = NUM_ROUNDS;
                next.player = 0;
                next.raises_this_round = 0;
                next.action_history = new_history;
                return next;
            }

            return next;
        }

    };

    // Verify the struct is exactly 16 bytes at compile time.
    static_assert(sizeof(GameState) == 16, "GameState must be exactly 16 bytes");
    static_assert(alignof(GameState) == 16, "GameState must be 16-byte aligned");

} // namespace leduc
