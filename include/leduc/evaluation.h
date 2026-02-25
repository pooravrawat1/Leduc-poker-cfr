#pragma once

#include "game.h"
#include "cfr.h"
#include "agent.h"

namespace leduc
{

    /// Match result statistics.
    struct MatchResult
    {
        double player0_winrate;
        int player0_wins;
        int player1_wins;
        vector<double> payoffs; // payoffs[i] = payoff to player 0 in game i
    };

    /// Evaluator for exploitability and best-response computation.
    class Evaluator
    {
    public:
        /// Create evaluator for given game and strategy.
        Evaluator(const LeducGame &game, const InfoSetTable &strategy_sum_table);

        /// Compute total exploitability (sum of best-response values).
        double compute_exploitability();

        /// Compute best-response value for defending against a given player.
        double compute_best_response_value(int defending_player);

    private:
        const LeducGame &game_;
        const InfoSetTable &strategy_sum_table_;

        /// Recursive best-response value computation.
        double br_value_recursive(const LeducState &state, int defending_player);
    };

    /// Play a match between two agents.
    /// @param agent_a First agent
    /// @param agent_b Second agent
    /// @param num_games Number of games to play
    /// @param seed Random seed
    /// @return Match result (agent_a is player 0)
    MatchResult play_match(Agent &agent_a, Agent &agent_b, int num_games, int seed = 42);

} // namespace leduc
