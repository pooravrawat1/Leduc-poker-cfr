#include "leduc/agent.h"
#include "leduc/strategy.h"

namespace leduc
{

    CFRAgent::CFRAgent(const InfoSetTable &strategy_sum_table, int seed)
        : strategy_sum_table_(strategy_sum_table), rng_(seed) {}

    string CFRAgent::act(const LeducState &state)
    {
        throw runtime_error("Not implemented in Phase 6");
    }

    string CFRAgent::sample_action(const unordered_map<string, double> &probs)
    {
        throw runtime_error("Not implemented in Phase 6");
    }

    RandomAgent::RandomAgent(int seed) : rng_(seed) {}

    string RandomAgent::act(const LeducState &state)
    {
        throw runtime_error("Not implemented in Phase 6");
    }

} // namespace leduc
