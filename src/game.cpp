#include "leduc/game.h"

namespace leduc
{

    bool LeducState::is_terminal() const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    bool LeducState::is_chance_node() const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    vector<string> LeducState::legal_actions() const
    {
        throw runtime_error("Not implemented in Phase 1");
    }

    LeducState LeducState::apply_action(const string &action) const
    {
        throw runtime_error("Not implemented in Phase 1");
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
        throw runtime_error("Not implemented in Phase 1");
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
