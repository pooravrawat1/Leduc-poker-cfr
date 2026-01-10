import numpy as np
from rlcard.utils.utils import remove_illegal

from cfr.tables import (
    regret_table,
    strategy_sum,
    make_infoset_key,
    regret_matching,
    mask_illegal_actions,
    update_strategy_sum
)


def cfr(env, state, player, reach_probs):
    if state['terminated']:
        payoffs = state['payoffs']
        return payoffs[player]

    current_player = state['current_player']
    legal_actions = list(state['legal_actions'].keys())

    if current_player == -1:
        value = 0.0
        for action, prob in env.get_chance_probabilities().items():
            next_state, _, _ = env.step(action)
            value += prob * cfr(env, next_state, player, reach_probs)
        return value

    raw = state['raw_obs']
    infoset_key = make_infoset_key(raw)

    regrets = regret_table[infoset_key]
    strategy = regret_matching(regrets)
    strategy = mask_illegal_actions(strategy, legal_actions)

    if current_player == player:
        update_strategy_sum(
            infoset_key,
            strategy,
            reach_probs[player]
        )

    action_utils = np.zeros(len(strategy), dtype=np.float32)
    node_utility = 0.0

    for action in legal_actions:
        next_reach = reach_probs.copy()
        next_reach[current_player] *= strategy[action]

        next_state, _, _ = env.step(action)
        util = cfr(env, next_state, player, next_reach)

        action_utils[action] = util
        node_utility += strategy[action] * util

        env.step_back()

    if current_player == player:
        opponent = 1 - player
        cf_reach = reach_probs[opponent]

        for action in legal_actions:
            regret_table[infoset_key][action] += (
                cf_reach * (action_utils[action] - node_utility)
            )

    return node_utility
