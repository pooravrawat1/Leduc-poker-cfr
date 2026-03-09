#include "leduc/infoset.h"

namespace leduc
{

    /// Mapping from raw bit-packed action sequence to the 4-bit ID used in Field A.
    /// Action encoding: FOLD=0, CALL=1, RAISE=2.
    static uint8_t encode_action_seq(uint8_t raw) noexcept
    {
        switch (raw)
        {
        case 0x00:
            return 0; // (empty)
        case 0x01:
            return 1; // Check (C)
        case 0x02:
            return 2; // Bet (R)
        case 0x05:
            return 3; // Check-Check (CC)
        case 0x06:
            return 4; // Check-Bet (CR)
        case 0x19:
            return 5; // Check-Bet-Call (CRC)
        case 0x1A:
            return 6; // Check-Bet-Raise (CRR)
        case 0x09:
            return 7; // Bet-Call (RC)
        case 0x0A:
            return 8; // Bet-Raise (RR)
        case 0x29:
            return 9; // Bet-Raise-Call (RRC)
        case 0x69:
            return 10; // Check-Bet-Raise-Call (CRRC)
        default:
            return 0;
        }
    }

    /// Mapping from raw bit-packed action sequence to the 3-bit ID used in Field B.
    /// Only terminal sequences (where the round ends) are mapped here.
    static uint8_t encode_terminal_seq(uint8_t raw) noexcept
    {
        switch (raw)
        {
        case 0x05:
            return 1; // Check-Check (CC)
        case 0x09:
            return 2; // Bet-Call (RC)
        case 0x19:
            return 3; // Check-Bet-Call (CRC)
        case 0x29:
            return 4; // Bet-Raise-Call (RRC)
        case 0x69:
            return 5; // Check-Bet-Raise-Call (CRRC)
        default:
            return 0;
        }
    }

    uint16_t compute_infoset_key(const GameState &state, uint8_t player) noexcept
    {
        uint16_t key = 0;

        // Field P: Current round (bits 14-15)
        key |= (uint16_t(state.round) << INFOSET_ROUND_SHIFT) & INFOSET_ROUND_MASK;

        // Field B: Betting history (bits 8-13)
        // bits [8..10] = round-0 terminal sequence
        // bits [11..13] = round-1 terminal sequence (usually 0 if in decision node)
        uint8_t hist_r0 = encode_terminal_seq(state.history_r0);
        uint16_t history = (uint16_t(hist_r0) << INFOSET_HIST_R0_SHIFT);
        // If we are in a terminal state for round 1, we might need hist_r1,
        // but compute_infoset_key is for decision nodes.
        key |= (history << INFOSET_HISTORY_SHIFT) & INFOSET_HISTORY_MASK;

        // Field C: Public card (bits 6-7)
        key |= (uint16_t(state.cards[2]) << INFOSET_PUBLIC_SHIFT) & INFOSET_PUBLIC_MASK;

        // Field H: Player's private card (bits 4-5)
        key |= (uint16_t(state.cards[player]) << INFOSET_CARD_SHIFT) & INFOSET_CARD_MASK;

        // Field A: Current round action sequence (bits 0-3)
        uint8_t action_id = encode_action_seq(state.action_history);
        key |= (uint16_t(action_id) << INFOSET_ACTION_SHIFT) & INFOSET_ACTION_MASK;

        return key;
    }

    void decode_infoset_key(uint16_t key,
                            uint8_t &out_round,
                            uint8_t &out_player_card,
                            uint8_t &out_public_card,
                            uint8_t &out_history,
                            uint8_t &out_action_seq) noexcept
    {
        out_round = static_cast<uint8_t>((key & INFOSET_ROUND_MASK) >> INFOSET_ROUND_SHIFT);
        out_player_card = static_cast<uint8_t>((key & INFOSET_CARD_MASK) >> INFOSET_CARD_SHIFT);
        out_public_card = static_cast<uint8_t>((key & INFOSET_PUBLIC_MASK) >> INFOSET_PUBLIC_SHIFT);
        out_history = static_cast<uint8_t>((key & INFOSET_HISTORY_MASK) >> INFOSET_HISTORY_SHIFT);
        out_action_seq = static_cast<uint8_t>((key & INFOSET_ACTION_MASK) >> INFOSET_ACTION_SHIFT);
    }

    std::string describe_infoset_key(uint16_t key)
    {
        uint8_t round, player_card, public_card, history, action_seq;
        decode_infoset_key(key, round, player_card, public_card, history, action_seq);

        // Card rank to character
        auto card_ch = [](uint8_t c) -> char
        {
            switch (c)
            {
            case CARD_J:
                return 'J';
            case CARD_Q:
                return 'Q';
            case CARD_K:
                return 'K';
            case CARD_NONE:
                return '-';
            default:
                return '?';
            }
        };

        // 3-bit terminal sequence id to short label
        auto seq3_label = [](uint8_t id) -> const char *
        {
            switch (id)
            {
            case 0:
                return "--";
            case 1:
                return "CC";
            case 2:
                return "RC";
            case 3:
                return "CRC";
            case 4:
                return "RRC";
            case 5:
                return "CRRC";
            default:
                return "?";
            }
        };

        // 4-bit current-round action sequence id to short label
        auto seq4_label = [](uint8_t id) -> const char *
        {
            switch (id)
            {
            case 0:
                return "(empty)";
            case 1:
                return "C";
            case 2:
                return "R";
            case 3:
                return "CC";
            case 4:
                return "CR";
            case 5:
                return "CRC";
            case 6:
                return "CRR";
            case 7:
                return "RC";
            case 8:
                return "RR";
            case 9:
                return "RRC";
            case 10:
                return "CRRC";
            default:
                return "?";
            }
        };

        uint8_t hist_r0 = history & static_cast<uint8_t>(INFOSET_HIST_R0_MASK);
        uint8_t hist_r1 = static_cast<uint8_t>((history & static_cast<uint8_t>(INFOSET_HIST_R1_MASK)) >> INFOSET_HIST_R1_SHIFT);

        std::string out;
        out.reserve(48);
        out += "R";
        out += static_cast<char>('0' + round);
        out += ' ';
        out += card_ch(player_card);
        out += '/';
        out += card_ch(public_card);
        out += " hist=";
        out += seq3_label(hist_r0);
        out += '|';
        out += seq3_label(hist_r1);
        out += " act=";
        out += seq4_label(action_seq);
        return out;
    }

} // namespace leduc
