#pragma once

#include "cfr.h"

namespace leduc
{

    /// Regret matching: convert regrets to strategy.
    /// Clamps negative regrets to 0; returns uniform if all ≤ 0; normalizes positive.
    unordered_map<string, double> regret_match(
        const unordered_map<string, double> &regrets);

    /// Get current strategy from regret table via regret matching.
    unordered_map<string, double> get_current_strategy(
        const string &info_set,
        const vector<string> &legal_actions,
        const InfoSetTable &regret_table);

    /// Get average strategy from strategy sum table via normalization.
    /// Fallback to uniform if sum is zero.
    unordered_map<string, double> get_average_strategy(
        const string &info_set,
        const vector<string> &legal_actions,
        const InfoSetTable &strategy_sum_table);

    /// Save strategy (strategy_sum_table) to JSON file.
    void save_strategy(const InfoSetTable &strategy_sum_table, const string &path);

    /// Load strategy from JSON file.
    InfoSetTable load_strategy(const string &path);

} // namespace leduc
