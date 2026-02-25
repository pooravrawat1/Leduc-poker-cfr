#include "leduc/strategy.h"

namespace leduc
{

    unordered_map<string, double> regret_match(
        const unordered_map<string, double> &regrets)
    {
        throw runtime_error("Not implemented in Phase 3");
    }

    unordered_map<string, double> get_current_strategy(
        const string &info_set,
        const vector<string> &legal_actions,
        const InfoSetTable &regret_table)
    {
        throw runtime_error("Not implemented in Phase 3");
    }

    unordered_map<string, double> get_average_strategy(
        const string &info_set,
        const vector<string> &legal_actions,
        const InfoSetTable &strategy_sum_table)
    {
        throw runtime_error("Not implemented in Phase 3");
    }

    void save_strategy(const InfoSetTable &strategy_sum_table, const string &path)
    {
        throw runtime_error("Not implemented in Phase 3 - strategy save");
    }

    InfoSetTable load_strategy(const string &path)
    {
        throw runtime_error("Not implemented in Phase 3 - strategy load");
    }

} // namespace leduc
