#pragma once

#include "game.h"

namespace leduc
{

    /// Tabular storage for info-set values (regrets or strategy sums).
    /// Backed by nested unordered_map: info_set -> action -> value
    class InfoSetTable
    {
    public:
        /// Get value for (info_set, action). Returns 0.0 if not found.
        double get(const string &info_set, const string &action) const;

        /// Update value for (info_set, action) by adding delta.
        void update(const string &info_set, const string &action, double delta);

        /// Get all action values for an info_set.
        unordered_map<string, double> get_all(const string &info_set) const;

        /// Save to JSON file.
        void save(const string &path) const;

        /// Load from JSON file (static factory).
        static InfoSetTable load(const string &path);

        /// Clear all data.
        void clear();

        /// Get number of stored info-sets.
        size_t size() const;

    private:
        unordered_map<string, unordered_map<string, double>> table_;
    };

    /// Vanilla Counterfactual Regret Minimization solver.
    /// Single-threaded; designed for SIMD/threading extension.
    class VanillaCFR
    {
    public:
        /// Create solver for the given game.
        VanillaCFR(const LeducGame &game);

        /// Main CFR recursion: compute utilities and update regrets/strategy sums.
        /// @param state Current game state
        /// @param player Player to compute utility for (0 or 1)
        /// @param reach_p0 Probability of reaching this state via player 0's actions
        /// @param reach_p1 Probability of reaching this state via player 1's actions
        /// @return Utility for `player` at this state
        double cfr(const LeducState &state, int player, double reach_p0, double reach_p1);

        /// Run one complete training iteration.
        /// Calls cfr() for both players with alternating updates.
        void train_iteration();

        /// Get regret table (const reference).
        const InfoSetTable &get_regret_table() const { return regret_table_; }

        /// Get strategy sum table (const reference).
        const InfoSetTable &get_strategy_sum_table() const { return strategy_sum_table_; }

        /// Get regret table (mutable reference for external updates).
        InfoSetTable &get_regret_table_mut() { return regret_table_; }

        /// Get strategy sum table (mutable reference for external updates).
        InfoSetTable &get_strategy_sum_table_mut() { return strategy_sum_table_; }

    private:
        const LeducGame &game_;
        InfoSetTable regret_table_;
        InfoSetTable strategy_sum_table_;
    };

} // namespace leduc
