import numpy as np
from collections import defaultdict
NUM_ACTIONS = 3

regret_table = defaultdict(lambda: np.zeros(NUM_ACTIONS, dtype=np.float32))
strategy_sum = defaultdict(lambda: np.zeros(NUM_ACTIONS, dtype=np.float32))

def make_infoset_key(raw_obs):
    private_card = raw_obs['hand'][0]
    public_card = raw_obs.get('public_card')
    round_idx = raw_obs['round']
    raise_count = raw_obs['raise_count']

    return (
        private_card,
        public_card if public_card is not None else 'None',
        round_idx,
        raise_count
    )
def regret_matching(regrets):
    positive_regrets = np.maximum(regrets, 0.0)
    total = positive_regrets.sum()

    if total > 0:
        return positive_regrets / total

    return np.ones(NUM_ACTIONS, dtype=np.float32) / NUM_ACTIONS


def mask_illegal_actions(strategy, legal_actions):
    mask = np.zeros(NUM_ACTIONS, dtype=np.float32)
    for action in legal_actions:
        mask[action] = 1.0

    masked = strategy * mask
    total = masked.sum()

    if total > 0:
        return masked / total

    return mask / mask.sum()

def update_strategy_sum(infoset_key, strategy, reach_prob):
    """
    Accumulate strategy for averaging.
    """
    strategy_sum[infoset_key] += reach_prob * strategy


def get_average_strategy(infoset_key):
    total = strategy_sum[infoset_key].sum()
    if total > 0:
        return strategy_sum[infoset_key] / total

    return np.ones(NUM_ACTIONS, dtype=np.float32) / NUM_ACTIONS
