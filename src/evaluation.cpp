#include "leduc/evaluation.h"

namespace leduc
{

    Evaluator::Evaluator(const LeducGame &game, const InfoSetTable &strategy_sum_table)
        : game_(game), strategy_sum_table_(strategy_sum_table) {}

    double Evaluator::compute_exploitability()
    {
        throw runtime_error("Not implemented in Phase 6");
    }

    double Evaluator::compute_best_response_value(int defending_player)
    {
        LeducState root = game_.initial_state();
        return br_value_recursive(root, defending_player);
    }

    double Evaluator::br_value_recursive(const LeducState &state, int defending_player)
    {
        throw runtime_error("Not implemented in Phase 6");
    }

    MatchResult play_match(Agent &agent_a, Agent &agent_b, int num_games, int seed)
    {
        throw runtime_error("Not implemented in Phase 6");
    }

} // namespace leduc
