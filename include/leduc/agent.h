#pragma once

#include "game.h"
#include "cfr.h"

namespace leduc
{

    /// Base class for agents.
    class Agent
    {
    public:
        virtual ~Agent() = default;

        /// Select an action for the given state.
        virtual string act(const LeducState &state) = 0;
    };

    /// Agent that plays according to CFR solution (average strategy).
    class CFRAgent : public Agent
    {
    public:
        /// Create agent from strategy sum table.
        explicit CFRAgent(const InfoSetTable &strategy_sum_table, int seed = 42);

        /// Select action by sampling from average strategy.
        string act(const LeducState &state) override;

    private:
        const InfoSetTable &strategy_sum_table_;
        mt19937 rng_;

        /// Sample action proportional to probabilities.
        string sample_action(const unordered_map<string, double> &probs);
    };

    /// Agent that plays uniformly at random.
    class RandomAgent : public Agent
    {
    public:
        explicit RandomAgent(int seed = 42);

        /// Select uniformly random legal action.
        string act(const LeducState &state) override;

    private:
        mt19937 rng_;
    };

} // namespace leduc
