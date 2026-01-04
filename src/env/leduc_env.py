import numpy as np

CARD_RANKS = ['J', 'Q', 'K']
NUM_CARDS = 6


def one_hot(index, size):
    vec = np.zeros(size, dtype=np.float32)
    if index is not None:
        vec[index] = 1.0
    return vec


def extract_state(state, player_id):
    """
    Convert RLCard Leduc state into an information-set tensor.
    """
    obs = state['obs']
    legal_actions = state['legal_actions']

    # ---------- Private card ----------
    private_card = state['raw_obs']['hand'][0]
    private_idx = CARD_RANKS.index(private_card[0])
    private_onehot = one_hot(private_idx * 2, NUM_CARDS)

    # ---------- Public card ----------
    public_card = state['raw_obs'].get('public_card')
    if public_card is None:
        public_onehot = np.zeros(NUM_CARDS + 1, dtype=np.float32)
        public_onehot[-1] = 1.0  # not revealed
    else:
        public_idx = CARD_RANKS.index(public_card[0])
        public_onehot = one_hot(public_idx * 2, NUM_CARDS + 1)

    # ---------- Betting round ----------
    round_idx = state['raw_obs']['round']
    round_onehot = one_hot(round_idx, 2)

    # ---------- Betting summary ----------
    chips = state['raw_obs']['chips']
    pot = sum(chips)

    betting_features = np.array([
        chips[player_id] / 10.0,
        chips[1 - player_id] / 10.0,
        pot / 20.0,
        state['raw_obs']['raise_count'] / 2.0
    ], dtype=np.float32)

    # ---------- Player indicator ----------
    player_feat = np.array([1.0], dtype=np.float32)

    # ---------- Final state ----------
    state_vec = np.concatenate([
        private_onehot,
        public_onehot,
        round_onehot,
        betting_features,
        player_feat
    ])

    return state_vec, legal_actions