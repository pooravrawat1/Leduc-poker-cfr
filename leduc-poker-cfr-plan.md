## Plan: Leduc Poker CFR Solver — Full Engineering Design

**TL;DR:** Build a research-grade, pure-Python (numpy + matplotlib only) Leduc Poker solver using vanilla CFR, entirely from scratch with zero external game or ML libraries. The existing codebase is a near-empty skeleton — only `extract_state()` exists and will be removed along with RLCard/PyTorch dependencies. The new implementation follows a strict module hierarchy: game engine → CFR tables → CFR recursion → training loop → evaluation → CLI. Every module is tabular; Deep CFR infra (`advantage_net.py`, `replay_buffer.py`) is preserved as a stub for future extension.

---

### Phase 0 — Repo Cleanup & Dependency Reset

**Steps**

1. Rewrite `requirements.txt`: remove `torch`, `rlcard`, keep `numpy`, `matplotlib`, add `dataclasses` (stdlib note only), `argparse` (stdlib).
2. Rewrite `setup.py` with package name, version, and `find_packages(src)`.
3. Delete the body of `src/env/leduc_env.py` — the RLCard-dependent `extract_state()` will be replaced by the new game engine. Keep the file as the home for the pure-Python `LeducEnv` wrapper.
4. Create `tests/` directory (currently absent) with `__init__.py`, `test_game.py`, `test_cfr.py`, `test_evaluation.py`.
5. Rewrite `notes.md` as a running engineering log.

---

### Phase 1 — Game Engine (`src/env/`)

This is the foundation. CFR correctness depends entirely on a bug-free game state representation.

**`src/env/leduc_env.py` — `LeducGame` class**

6. Define constants: `RANKS = ['J', 'Q', 'K']`, `DECK` (6 cards — two of each rank), `BET_SIZES = {0: 2, 1: 4}` (round 0 / round 1), `MAX_RAISES_PER_ROUND = 1`.
7. Implement `LeducState` dataclass:
   - Fields: `private_cards: tuple[str, str]`, `public_card: Optional[str]`, `history: list[str]` (per-round action sequences), `current_round: int`, `current_player: int`, `pot: int`, `chips_to_call: int`, `raises_this_round: int`, `folded: Optional[int]`.
   - Methods:
     - `is_terminal() → bool`
     - `current_player() → int`
     - `is_chance_node() → bool` (before cards are dealt or before public card is revealed)
     - `legal_actions() → list[str]` — returns subset of `['fold', 'call', 'check', 'bet', 'raise']` based on game state
     - `apply_action(action: str) → LeducState` — returns new state (immutable updates)
     - `utility(player: int) → float` — only valid on terminal states; implements Leduc showdown rules (pair > high card; J < Q < K)
     - `info_set_key(player: int) → str` — encodes imperfect information as `"{private_card}|{public_card_or_?}|{round0_history}/{round1_history}"`

8. Implement showdown logic precisely:
   - If `folded is not None`: non-folding player wins the pot.
   - Else: pair (private == public) beats non-pair; among same category, higher rank wins; split pot on tie.
9. Implement chance sampling: `LeducGame.deal_chance(state) → list[(action, prob)]` — returns all possible private/public card deals with uniform probability.

**`src/env/state_encoder.py` — for future Deep CFR extension**

10. Stub `encode_state(state: LeducState, player: int) → np.ndarray` — 20-dim float32 vector matching the existing spec, but corrected: fix the sparse one-hot bug (use indices 0–5 directly, not `*2`), fix the player indicator feature.

**Tests** (in `tests/test_game.py`):

11. Test `legal_actions()` at root, after bet, after raise, after max raises.
12. Test `apply_action()` transitions: fold → terminal, call → next player or round advance.
13. Test `utility()` on a) fold scenario, b) pair win, c) high-card win, d) split pot.
14. Test `info_set_key()` returns identical keys for two histories that produce the same information set.

---

### Phase 2 — CFR Tables (`src/cfr/tables.py`)

15. Implement `InfoSetTable`:

- Backed by `collections.defaultdict(lambda: defaultdict(float))`.
- Methods: `get(info_set, action) → float`, `update(info_set, action, delta)`, `get_all(info_set) → dict[str, float]`, `save(path)` / `load(path)` using `json`.

16. Implement two instances managed by the solver: `regret_table: InfoSetTable` (cumulative counterfactual regrets) and `strategy_sum_table: InfoSetTable` (cumulative weighted strategies).

---

### Phase 3 — Strategy Extraction (`src/cfr/strategy.py`)

17. Implement `regret_match(regrets: dict[str, float]) → dict[str, float]`:

- Clip negatives to 0.
- If all ≤ 0, return uniform over actions.
- Else normalize positive regrets.

18. Implement `get_current_strategy(info_set, legal_actions, regret_table) → dict[str, float]` — calls `regret_match`.
19. Implement `get_average_strategy(info_set, legal_actions, strategy_sum_table) → dict[str, float]` — normalizes strategy sums; fallback to uniform if zero.
20. Implement `save_strategy(strategy_sum_table, path: str)` / `load_strategy(path: str) → InfoSetTable`.

---

### Phase 4 — Vanilla CFR Solver (`src/cfr/cfr.py`)

This is the core research component.

21. Implement `VanillaCFR` class with:

- Constructor: takes `game: LeducGame`, `regret_table`, `strategy_sum_table`.
- `cfr(state: LeducState, player: int, reach_p0: float, reach_p1: float) → float`:
  - **Terminal**: return `state.utility(player)`.
  - **Chance node**: enumerate all card deals w/ their probabilities; recurse with scaled reach; return probability-weighted average.
  - **Decision node** (for `current_player`):
    - Retrieve `strategy = get_current_strategy(info_set, legal_actions, regret_table)`.
    - For each action: recurse to get `action_value`.
    - Compute `node_value = sum(strategy[a] * action_value[a])`.
    - **Regret update** (only for `current_player == player`): `regret_table.update(info_set, a, opponent_reach * (action_value[a] - node_value))`.
    - **Strategy sum update**: `strategy_sum_table.update(info_set, a, player_reach * strategy[a])`.
    - Return `node_value`.
- `train_iteration()`: calls `cfr(root_state, player=0, 1.0, 1.0)` then `cfr(root_state, player=1, 1.0, 1.0)` — **alternating updates** (standard vanilla CFR).

**Important invariant to document:** reach probability of the traversing player propagates through their own nodes; opponent's reach propagates through opponent nodes and chance nodes.

---

### Phase 5 — Training Pipeline (`src/cfr/train.py` + `src/train.py`)

22. Implement `CFRTrainer` in `src/cfr/train.py`:

- Constructor: `num_iterations`, `checkpoint_interval`, `output_dir`, `seed`.
- `train() → TrainingResult` — runs `num_iterations` of `train_iteration()`; logs average game value per 1000 iterations; saves checkpoint every `checkpoint_interval` iterations.
- `TrainingResult`: dataclass with `regret_history`, `game_value_history`, `final_exploitability`.

23. Implement `src/utils/config.py`: `CFRConfig` dataclass — all hyperparameters with defaults (`num_iterations=100_000`, `checkpoint_interval=10_000`, `seed=42`, `output_dir='checkpoints/'`), plus `from_args(args) → CFRConfig` using `argparse`.
24. Implement `src/utils/logging.py`: `TrainingLogger` — writes CSV (`iteration, game_value, exploitability`) and prints tqdm-style progress since `tqdm` is available.
25. Implement `src/train.py`: entry-point CLI — `python src/train.py --iterations 100000 --output checkpoints/` — invokes `CFRConfig.from_args()` → `CFRTrainer(config).train()`.

---

### Phase 6 — Exploitability & Evaluation (`src/evaluation/`)

Exploitability quantifies how close to Nash equilibrium the learned strategy is — the primary research metric.

26. Implement `src/evaluation/exploitability.py` — `compute_exploitability(strategy_sum_table, game) → float`:

- For each player `p`, compute the **best response value** `br_value(p)` by walking the full game tree:
  - At opponent's nodes: follow opponent's average strategy (weighted average over actions).
  - At `p`'s nodes: take the `max` over all actions (best response).
  - At chance nodes: probability-weighted average.
- `exploitability = br_value(0) + br_value(1)` (in a zero-sum game this equals the sum of exploitabilities).

27. Implement `src/evaluation/play_match.py` — `run_match(agent_a, agent_b, num_games, seed) → MatchResult` using `LeducGame` without RLCard.
28. Implement `src/agent/cfr_agent.py` — `CFRAgent.act(state) → str`: looks up `get_average_strategy(state.info_set_key(player), ...)` and samples from the distribution.
29. Implement `src/agent/random_agent.py` — `RandomAgent.act(state) → str`: uniform sample from `state.legal_actions()`.
30. Implement `src/evaluation/metrics.py` and `src/utils/metrics.py`: win rate, average pot, fold frequency, bluff frequency by info-set.
31. Implement `scripts/evaluate.py`: loads checkpoint, runs `CFRAgent` vs `RandomAgent` for 10,000 games, prints win rate + exploitability.

**Tests** (`tests/test_evaluation.py`):

32. Test that `compute_exploitability` of a **uniform random** strategy is > 0.
33. Test that exploitability decreases monotonically (or near-monotonically) after N iterations.
34. Test `CFRAgent.act()` always returns a legal action.

---

### Phase 7 — Utils: Metrics & Checkpointing (`src/utils/`)

35. Implement `src/utils/metrics.py`: `RunningMean` tracker; `MetricsStore` dict accumulating per-iteration scalars.
36. Implement checkpointing in `TrainingLogger`: `save_checkpoint(iteration, regret_table, strategy_sum_table, path)` / `load_checkpoint(path) → (regret_table, strategy_sum_table, iteration)` using `json.dump/load`.

---

### Phase 8 — CLI Human Play Interface (`src/interface/cli_play.py`)

37. Implement `CLIGame` class:

- `play_game(agent: CFRAgent, human_player: int)` — drives a full interactive game loop.
- Displays: private card, public card when revealed, pot size, legal actions.
- Human inputs action via stdin; validates against `legal_actions()`.
- After terminal: shows result, cards, payoff.
- `play_session(agent, num_games)` — loops `play_game`, tracks session stats.

38. Implement entry point: `python src/interface/cli_play.py --checkpoint checkpoints/final.json --games 10`.

---

### Phase 9 — Visualization & Analytics (`experiments/plots.ipynb`)

39. Populate `experiments/plots.ipynb` with cells for:

- **Regret convergence**: plot `sum |regret_t|` vs iteration.
- **Exploitability curve**: plot exploitability (mBB/hand) vs iteration.
- **Strategy frequency heatmap**: for key info-sets (e.g., holding Q pre-flop), show action distribution evolution.
- **Win-rate vs random**: bar chart of CFR agent win rate at checkpoint intervals.

40. Implement `experiments/train_leduc.sh`: full training + evaluation shell script.

---

### Phase 10 — Documentation & README

41. Rewrite `README.md`: project overview, setup instructions, `python src/train.py` quickstart, `python src/interface/cli_play.py` usage, architecture diagram in ASCII.
42. Add docstrings to every public class and function.
43. Update `notes.md` as an engineering decision log.

---

### Stubs Preserved for Future Extension

- `src/models/advantage_net.py`: leave stubbed with a class docstring explaining its role in Deep CFR.
- `src/utils/replay_buffer.py`: leave stubbed for MCCFR / Deep CFR.
- `src/interface/web_app.py`: leave stubbed with a Flask route skeleton.

---

### Verification

- **Unit tests**: `python -m pytest tests/` — covers game engine, CFR updates, strategy normalization, exploitability, agent legality.
- **Convergence check**: run 100,000 iterations; expected exploitability < 0.05 (Leduc converges fast due to small game tree).
- **Smoke test**: `python scripts/evaluate.py --checkpoint checkpoints/100k.json` — should report CFR agent win rate > 55% vs random.
- **CLI test**: `python src/interface/cli_play.py --checkpoint checkpoints/100k.json --games 1` — play one hand manually.
- **Plot notebook**: execute all cells; confirm all four charts render without error.

---

### Milestones

| #   | Milestone                 | Deliverable                                                        |
| --- | ------------------------- | ------------------------------------------------------------------ |
| M1  | Game engine complete      | All `test_game.py` tests pass; `LeducState` handles full game tree |
| M2  | CFR tables + strategy     | `regret_match` unit tests pass; save/load round-trips              |
| M3  | Vanilla CFR running       | `train_iteration()` executes without error; regret table populates |
| M4  | Training pipeline         | `python src/train.py` runs 10k iterations, writes checkpoint       |
| M5  | Exploitability measurable | `compute_exploitability` returns < 0.05 at 100k iterations         |
| M6  | CLI playable              | Human can play a full game vs trained agent                        |
| M7  | Plots & docs              | `plots.ipynb` renders; README is complete                          |

---

### Decisions

- **Pure from-scratch**: Remove `rlcard` and `torch`; `requirements.txt` will only contain `numpy` and `matplotlib`.
- **String keys for info-sets**: Tabular CFR uses `"{private}|{public_or_?}|{round0_hist}/{round1_hist}"` — human-readable, debuggable, no encoding bugs.
- **Alternating player updates**: Both players updated per iteration (standard vanilla CFR); faster convergence than simultaneous.
- **Immutable `LeducState`**: `apply_action()` returns a new state object — correct for recursive CFR; no undo stack needed.
- **`LeducGame` as `src/env/` not `src/game/`**: The existing directory structure is kept; adding a `src/game/` directory is not necessary and would require refactoring all existing imports.
- **Exploitability via full tree traversal**: Feasible for Leduc (small game tree ~3 million nodes at most); no approximation needed.
- **Deep CFR stubs retained**: `advantage_net.py` and `replay_buffer.py` remain as documented stubs — architecture explicitly supports future extension.
