#include "leduc/cfr.h"

namespace leduc
{

    double InfoSetTable::get(const string &info_set, const string &action) const
    {
        auto it = table_.find(info_set);
        if (it == table_.end())
            return 0.0;
        auto it2 = it->second.find(action);
        if (it2 == it->second.end())
            return 0.0;
        return it2->second;
    }

    void InfoSetTable::update(const string &info_set, const string &action, double delta)
    {
        table_[info_set][action] += delta;
    }

    unordered_map<string, double> InfoSetTable::get_all(const string &info_set) const
    {
        auto it = table_.find(info_set);
        if (it == table_.end())
            return {};
        return it->second;
    }

    void InfoSetTable::save(const string &path) const
    {
        throw runtime_error("Not implemented in Phase 2 - JSON serialization");
    }

    InfoSetTable InfoSetTable::load(const string &path)
    {
        throw runtime_error("Not implemented in Phase 2 - JSON deserialization");
    }

    void InfoSetTable::clear()
    {
        table_.clear();
    }

    size_t InfoSetTable::size() const
    {
        return table_.size();
    }

    VanillaCFR::VanillaCFR(const LeducGame &game) : game_(game) {}

    double VanillaCFR::cfr(const LeducState &state, int player, double reach_p0, double reach_p1)
    {
        throw runtime_error("Not implemented in Phase 4 - CFR recursion");
    }

    void VanillaCFR::train_iteration()
    {
        throw runtime_error("Not implemented in Phase 4 - train_iteration");
    }

} // namespace leduc
