import rlcard
from src.env.leduc_env import extract_state

env = rlcard.make('leduc-holdem')
env.reset()

player_id = env.get_player_id()
state = env.get_state(player_id)

vec, legal = extract_state(state, player_id)

print("State vector shape:", vec.shape)
print("State vector:", vec)
print("Legal actions:", legal)
