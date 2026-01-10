import rlcard
import numpy as np
from tqdm import trange

from cfr.cfr import cfr
from cfr.tables import regret_table, strategy_sum


def train_cfr(
    num_iterations=50000,
    print_every=5000,
    seed=42
):

    env = rlcard.make('leduc-holdem')
    env.set_seed(seed)

    for iteration in trange(1, num_iterations + 1):
        for player in [0, 1]:
            env.reset()
            state = env.get_state(player)
            reach_probs = [1.0, 1.0]

            cfr(env, state, player, reach_probs)

        if iteration % print_every == 0:
            avg_regret = np.mean([
                np.sum(np.maximum(r, 0))
                for r in regret_table.values()
            ])
            print(
                f"Iteration {iteration} | "
                f"Avg positive regret: {avg_regret:.4f} | "
                f"Infosets: {len(regret_table)}"
            )

    print("Training complete.")
    return env


if __name__ == "__main__":
    train_cfr()