#include "leduc/game.h"

namespace leduc
{

    bool LeducState::is_terminal() const
    {
        if (folded.has_value())
            return true;

        // Game ends after Round 1 is settled
        if (current_round == 1 && current_player != -1)
        {
            const vector<string> &actions = history[1];
            if (actions.size() >= 2)
            {
                string last = actions.back();
                string prev = actions[actions.size() - 2];

                // Settled if:
                // 1. Both checked: "check", "check"
                // 2. Someone called a bet/raise: "...", "call"
                if (last == "check" && prev == "check")
                    return true;
                if (last == "call")
                    return true;
            }
        }
        return false;
    }

    bool LeducState::is_chance_node() const
    {
        return current_player == -1;
    }

    LeducState LeducState::apply_action(const string &action) const
    {
        LeducState next = *this;
        next.history[current_round].push_back(action);

        if (action == "fold")
        {
            next.folded = current_player;
        }
        else if (action == "check")
        {
            if (next.history[current_round].size() == 2)
            {
                // Both checked, move to next round or terminal
                if (current_round == 0)
                {
                    next.current_round = 1;
                    next.current_player = -1; // Chance node to reveal public card
                }
            }
            else
            {
                next.current_player = 1 - current_player;
            }
        }
        else if (action == "bet")
        {
            int amount = BET_SIZES[current_round];
            next.pot += amount;
            next.chips_to_call = amount;
            next.current_player = 1 - current_player;
        }
        else if (action == "call")
        {
            next.pot += next.chips_to_call;
            next.chips_to_call = 0;

            // Round ends after a call
            if (current_round == 0)
            {
                next.current_round = 1;
                next.current_player = -1; // Chance node
            }
        }
        else if (action == "raise")
        {
            int amount = next.chips_to_call + BET_SIZES[current_round];
            next.pot += amount;
            next.chips_to_call = BET_SIZES[current_round];
            next.raises_this_round++;
            next.current_player = 1 - current_player;
        }

        return next;
    }

    double LeducState::utility(int player) const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    string LeducState::info_set_key(int player) const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    LeducState LeducGame::initial_state() const
    {
        LeducState state;
        state.private_cards = {'\0', '\0'};
        state.public_card = nullopt;
        state.history = {{}, {}};
        state.current_round = 0;
        state.current_player = -1; // chance node: cards not yet dealt
        state.pot = 2;             // both players post ante of 1
        state.chips_to_call = 0;
        state.raises_this_round = 0;
        state.folded = nullopt;
        return state;
    }

    vector<pair<string, double>> LeducGame::sample_chance(
        const LeducState &state) const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    vector<LeducState> LeducGame::get_all_hands() const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

} // namespace leduc
