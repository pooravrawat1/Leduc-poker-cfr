#pragma once

#include <cstdint>
#include <string>
#include "types.h"
#include "game_state.h"

// =============================================================================
// Information Set Encoding – 16-bit packed key
// =============================================================================
//
// An "information set" is everything a player legitimately knows at their
// turn: their private card, the public card (if revealed), and the full
// betting history of the hand.  Each unique combination maps to one CFRNode.
//
// Leduc Poker has ≈936 reachable information sets.  We encode all the
// relevant data into a single uint16_t so that looking up or updating a node
// is a direct array index – zero pointer chasing, zero hashing.
//
// ─────────────────────────────────────────────────────────────────────────────
// Bit Layout  (uint16_t, 16 bits total)
// ─────────────────────────────────────────────────────────────────────────────
//
//  15 14 | 13 12 11 10  9  8 | 7  6 | 5  4 | 3  2  1  0
//  ──────┼───────────────────┼──────┼──────┼────────────
//   P  P |  B  B  B  B  B  B | C  C | H  H | A  A  A  A
//
//  Field          Bits    Width   Description
//  ─────────────  ──────  ─────   ──────────────────────────────────────────
//  A (action seq) [0..3]   4 bit  Current-round action sequence (see below)
//  H (player card)[4..5]   2 bit  Private card of the acting player
//                                  0=J, 1=Q, 2=K  (CARD_J/Q/K constants)
//  C (public card)[6..7]   2 bit  Community card
//                                  0=J, 1=Q, 2=K, 3=none (CARD_NONE)
//  B (bet history)[8..13]  6 bit  Betting history across BOTH rounds
//                                  bits [8..10] = round-0 sequence (3 bits)
//                                  bits [11..13] = round-1 sequence (3 bits)
//  P (round)      [14..15] 2 bit  Current round (0=pre-flop, 1=flop)
//
//  Total: 4+2+2+6+2 = 16 bits  → fits in uint16_t, max value ≤ 0xFFFF
//  Used range: 0 .. MAX_INFOSETS-1 (MAX_INFOSETS = 1024, defined in types.h)
//
// ─────────────────────────────────────────────────────────────────────────────
// Action Sequence Encoding  (4-bit field, bits [0..3])
// ─────────────────────────────────────────────────────────────────────────────
//
//  These represent the sequence of actions taken SO FAR in the current round.
//  Each action occupies 2 bits (FOLD=00, CALL=01, RAISE=10).  Because Leduc
//  allows at most 2 actions per player per round the sequences stay ≤4 bits.
//
//  Value   Meaning
//  ─────   ───────────────────────
//  0x0     (empty – first to act)
//  0x1     Check  (CALL with to_call==0)
//  0x2     Bet    (RAISE with to_call==0)
//  0x3     Check–Check  (both checked, round ends)
//  0x4     Check–Bet
//  0x5     Check–Bet–Call
//  0x6     Check–Bet–Raise
//  0x7     Bet–Call
//  0x8     Bet–Raise
//  0x9     Bet–Raise–Call
//  ...     (up to 16 distinct sequences)
//
// ─────────────────────────────────────────────────────────────────────────────
// Betting History Encoding  (6-bit field, bits [8..13])
// ─────────────────────────────────────────────────────────────────────────────
//
//  Captures the *completed* action log for both rounds:
//    bits [8..10]  → round-0 terminal sequence (3 bits, 8 values)
//    bits [11..13] → round-1 terminal sequence (3 bits, 8 values)
//  This lets the round-1 infoset key reflect what happened pre-flop even
//  though those actions are no longer in the current-round action field.
//
// =============================================================================

namespace leduc
{

    // ---------------------------------------------------------------------------
    // Shift and mask constants for each field
    // ---------------------------------------------------------------------------

    // Field A: current-round action sequence  (bits 0–3)
    constexpr uint16_t INFOSET_ACTION_SHIFT = 0;
    constexpr uint16_t INFOSET_ACTION_MASK = 0x000F; // 0000 0000 0000 1111

    // Field H: private (hole) card of the acting player  (bits 4–5)
    constexpr uint16_t INFOSET_CARD_SHIFT = 4;
    constexpr uint16_t INFOSET_CARD_MASK = 0x0030; // 0000 0000 0011 0000

    // Field C: public community card  (bits 6–7)
    constexpr uint16_t INFOSET_PUBLIC_SHIFT = 6;
    constexpr uint16_t INFOSET_PUBLIC_MASK = 0x00C0; // 0000 0000 1100 0000

    // Field B: full betting history across both rounds  (bits 8–13)
    constexpr uint16_t INFOSET_HISTORY_SHIFT = 8;
    constexpr uint16_t INFOSET_HISTORY_MASK = 0x3F00; // 0011 1111 0000 0000

    //   Sub-fields inside the history field (applied AFTER shifting by 8):
    constexpr uint16_t INFOSET_HIST_R0_SHIFT = 0; // bits [8..10]  → round 0
    constexpr uint16_t INFOSET_HIST_R0_MASK = 0x0007;
    constexpr uint16_t INFOSET_HIST_R1_SHIFT = 3; // bits [11..13] → round 1
    constexpr uint16_t INFOSET_HIST_R1_MASK = 0x0038;

    // Field P: current round  (bits 14–15)
    constexpr uint16_t INFOSET_ROUND_SHIFT = 14;
    constexpr uint16_t INFOSET_ROUND_MASK = 0xC000; // 1100 0000 0000 0000

    // ---------------------------------------------------------------------------
    // Function declarations  (implemented in 1.3.2 and 1.3.3)
    // ---------------------------------------------------------------------------

    /// Compute the 16-bit information set key for `player` in `state`.
    /// Returns a value in [0, MAX_INFOSETS) suitable for direct array indexing.
    uint16_t compute_infoset_key(const GameState &state, uint8_t player) noexcept;

    /// Decode a 16-bit infoset key back into raw numeric components.
    /// Writes results into the provided out-parameters (all non-null expected).
    /// Useful for debugging and logging only – not called in the hot loop.
    void decode_infoset_key(uint16_t key,
                            uint8_t &out_round,
                            uint8_t &out_player_card,
                            uint8_t &out_public_card,
                            uint8_t &out_history,
                            uint8_t &out_action_seq) noexcept;

    /// Return a human-readable description of a 16-bit infoset key.
    /// Format: "R<round> <card>/<pub_card> hist=<r0_seq>|<r1_seq> act=<seq>"
    /// Example: "R0 K/-- hist=--|-- act=(empty)"
    ///          "R1 Q/J  hist=RC|-- act=C"
    /// Intended for debug logging and test diagnostics only.
    std::string describe_infoset_key(uint16_t key);

} // namespace leduc
