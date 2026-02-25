## Plan: Leduc Poker CFR Solver — Full Engineering Design (C++)

**TL;DR:** Build a research-grade, modern C++17 Leduc Poker solver using vanilla CFR, entirely from scratch with zero external game or ML libraries. Zero dependencies except `nlohmann/json` (header-only) for serialization. The new implementation follows a strict module hierarchy: game engine → CFR tables → CFR recursion → training loop → evaluation → CLI. Every module is tabular; Deep CFR infra is preserved as a stub for future extension. Build with CMake 3.15+; single-threaded first, designed for SIMD/threading extension.

---

### Phase 0 — Repo Cleanup & C++ Setup

**Steps**

1. Delete `requirements.txt` and `setup.py` — no longer needed.
2. Create `CMakeLists.txt` with:
   - `cmake_minimum_required(3.15)`
   - `project(leduc-poker-cfr CXX)`
   - `set(CMAKE_CXX_STANDARD 17)`
   - Target for main executable `cfr_train`
   - Target for test executable `cfr_tests`
   - Include `nlohmann/json` (single-header, downloaded during CMake build or pre-installed)
3. Reorganize directory structure:
   ```
   leduc-poker-cfr/
   ├── CMakeLists.txt
   ├── README.md
   ├── notes.md
   ├── include/
   │   ├── leduc/
   │   │   ├── game.h          (LeducState, LeducGame)
   │   │   ├── cfr.h           (InfoSetTable, VanillaCFR)
   │   │   ├── strategy.h      (regret_match, strategy extraction)
   │   │   ├── agent.h         (CFRAgent, RandomAgent)
   │   │   └── evaluation.h    (exploitability, metrics)
   ├── src/
   │   ├── game.cpp
   │   ├── cfr.cpp
   │   ├── strategy.cpp
   │   ├── agent.cpp
   │   ├── evaluation.cpp
   │   ├── train.cpp           (main entry point for training)
   │   └── play.cpp            (main entry point for CLI play)
   ├── tests/
   │   ├── test_game.cpp
   │   ├── test_cfr.cpp
   │   └── test_evaluation.cpp
   ├── experiments/
   │   ├── train.sh            (build & train script)
   │   └── analyze.py          (Python plotting for results)
   └── data/
       └── (checkpoint .json files)
   ```
4. Create `include/leduc/*.h` with class declarations (no implementations yet).
5. Create `src/*.cpp` with stub implementations (method bodies return `NotImplementedError` equivalent or `throw std::runtime_error`).
6. Create `CMakeLists.txt` for GCC/Clang with flags: `-Wall -Wextra -Werror -O2`.
7. Create minimal `tests/CMakeLists.txt` or embed in root `CMakeLists.txt` with Google Test or custom test runner.
8. Rewrite `notes.md` as running engineering log.
9. Delete `src/env/`, `src/cfr/`, `src/train.py`, `tests/` (old Python structure).

---

### Phase 1 — Game Engine (`include/leduc/game.h` + `src/game.cpp`)

This is the foundation. CFR correctness depends entirely on a bug-free game state representation.

**`include/leduc/game.h` — `LeducState` and `LeducGame` classes**

6. Define constants in header:

   ```cpp
   constexpr std::array<char, 3> RANKS = {'J', 'Q', 'K'};
   constexpr std::array<int, 2> BET_SIZES = {2, 4};  // round 0, round 1
   constexpr int MAX_RAISES_PER_ROUND = 1;
   constexpr int DECK_SIZE = 6;  // 2 of each rank
   ```

7. Implement `struct LeducState`:
   - Fields:
     ```cpp
     std::pair<char, char> private_cards;      // (p0 card, p1 card)
     std::optional<char> public_card;          // null in round 0
     std::vector<std::vector<std::string>> history;  // per-round actions
     int current_round;         // 0 or 1
     int current_player;        // 0 or 1
     int pot;
     int chips_to_call;
     int raises_this_round;
     std::optional<int> folded; // which player folded, if any
     ```
   - Methods:
     - `bool is_terminal() const`
     - `bool is_chance_node() const`
     - `std::vector<std::string> legal_actions() const`
     - `LeducState apply_action(const std::string& action) const` (returns new state)
     - `double utility(int player) const` (only on terminal states)
     - `std::string info_set_key(int player) const` (encodes imperfect information)

8. Implement showdown logic:
   - If player folded: other player wins pot.
   - Else: pair beats non-pair; higher rank wins; split on tie.

9. Implement `class LeducGame`:
   - `LeducState initial_state() const` — returns root state (pre-deal)
   - `std::vector<std::pair<std::string, double>> sample_chance(const LeducState&) const` — returns all card deals with probabilities
   - `std::vector<LeducState> get_all_hands() const` — enumerates all initial hands (for testing/exploitability)

**`src/game.cpp` — Implementations**

All method implementations (no stubs—implement fully in Phase 1).

**Tests** (`tests/test_game.cpp`):

10. Test `legal_actions()` at root, after bet, after raise, after max raises.
11. Test `apply_action()` transitions: fold → terminal, call → next player/round.
12. Test `utility()` on a) fold, b) pair win, c) high-card win, d) split pot.
13. Test `info_set_key()` produces identical keys for equivalent situations.

---

### Phase 2 — CFR Tables (`include/leduc/cfr.h` + `src/cfr.cpp`)

14. Implement `class InfoSetTable`:

- Backed by `std::unordered_map<std::string, std::unordered_map<std::string, double>>`.
- Methods:
  ```cpp
  double get(const std::string& info_set, const std::string& action) const;
  void update(const std::string& info_set, const std::string& action, double delta);
  std::unordered_map<std::string, double> get_all(const std::string& info_set) const;
  void save(const std::string& path) const;  // JSON serialization
  static InfoSetTable load(const std::string& path);
  ```

15. Implement two instances in CFR solver:

- `regret_table: InfoSetTable` (cumulative counterfactual regrets)
- `strategy_sum_table: InfoSetTable` (cumulative weighted strategies)

---

### Phase 3 — Strategy Extraction (`include/leduc/strategy.h` + `src/strategy.cpp`)

16. Implement utility functions:

```cpp
std::unordered_map<std::string, double> regret_match(
    const std::unordered_map<std::string, double>& regrets
);
// Clamps negative regrets to 0; returns uniform if all ≤ 0; normalizes positive.

std::unordered_map<std::string, double> get_current_strategy(
    const std::string& info_set,
    const std::vector<std::string>& legal_actions,
    const InfoSetTable& regret_table
);

std::unordered_map<std::string, double> get_average_strategy(
    const std::string& info_set,
    const std::vector<std::string>& legal_actions,
    const InfoSetTable& strategy_sum_table
);

void save_strategy(const InfoSetTable& strategy_sum_table, const std::string& path);
InfoSetTable load_strategy(const std::string& path);
```

---

### Phase 4 — Vanilla CFR Solver (`include/leduc/cfr.h` + `src/cfr.cpp`)

This is the core research component.

17. Implement `class VanillaCFR`:

```cpp
class VanillaCFR {
public:
    VanillaCFR(const LeducGame& game);

    double cfr(const LeducState& state, int player, double reach_p0, double reach_p1);
    void train_iteration();

    const InfoSetTable& get_regret_table() const { return regret_table_; }
    const InfoSetTable& get_strategy_sum_table() const { return strategy_sum_table_; }

private:
    const LeducGame& game_;
    InfoSetTable regret_table_;
    InfoSetTable strategy_sum_table_;
};
```

18. Implement `VanillaCFR::cfr()`:

- **Terminal node**: return `state.utility(player)`
- **Chance node**: enumerate card deals; recurse with scaled reach; return probability-weighted utility
- **Decision node** (for `state.current_player()`):
  - Get `strategy = get_current_strategy(info_set, legal_actions, regret_table_)`
  - For each action: compute `action_value = cfr(next_state, player, ...)`
  - Compute `node_value = sum(strategy[a] * action_value[a])`
  - **Regret update** (only if `state.current_player() == player`):
    `regret_table_.update(info_set, a, opponent_reach * (action_value[a] - node_value))`
  - **Strategy sum update**:
    `strategy_sum_table_.update(info_set, a, player_reach * strategy[a])`
  - Return `node_value`

19. Implement `VanillaCFR::train_iteration()`:

- Call `cfr(game_.initial_state(), 0, 1.0, 1.0)` then `cfr(..., 1, 1.0, 1.0)`
- Standard vanilla CFR with alternating player updates per iteration

**Important invariant:** Own player's reach probability flow through their own decision nodes; opponent's reach through opponent's nodes and chance nodes.

### Phase 5 — Training Pipeline (`src/train.cpp` + supporting helpers)

20. Implement `class CFRTrainer`:

```cpp
struct CFRConfig {
    int num_iterations = 100000;
    int checkpoint_interval = 10000;
    std::string output_dir = "checkpoints";
    int seed = 42;
};

class CFRTrainer {
public:
    CFRTrainer(const LeducGame& game, const CFRConfig& config);
    void train();
    void save_checkpoint(int iteration) const;

private:
    const LeducGame& game_;
    CFRConfig config_;
    VanillaCFR solver_;
    std::ofstream csv_log_;
};
```

21. Implement training loop:

- For each iteration: `solver_.train_iteration()`
- Log average game value every 1000 iterations
- Save checkpoint every `checkpoint_interval` iterations
- Use `std::chrono` for timing

22. Implement `src/train.cpp` entry point:

```cpp
int main(int argc, char* argv[]) {
    // Parse command-line: --iterations, --output, --seed (using simple arg parsing or getopt)
    CFRConfig config = parse_args(argc, argv);
    LeducGame game;
    CFRTrainer trainer(game, config);
    trainer.train();
    return 0;
}
```

- Usage: `./cfr_train --iterations 100000 --output checkpoints/`

---

### Phase 6 — Exploitability & Evaluation (`include/leduc/evaluation.h` + `src/evaluation.cpp`)

Exploitability quantifies how close to Nash equilibrium the learned strategy is — the primary research metric.

23. Implement `class Evaluator`:

```cpp
class Evaluator {
public:
    Evaluator(const LeducGame& game, const InfoSetTable& strategy_sum_table);
    double compute_exploitability();
    double compute_best_response_value(int player_to_defend_against);

private:
    const LeducGame& game_;
    const InfoSetTable& strategy_sum_table_;
    double br_value_recursive(const LeducState& state, int defending_player);
};
```

24. Implement best-response (BR) value computation:

- At opponent's nodes: follow opponent's average strategy (sum over actions)
- At defending player's nodes: take `max` over all actions
- At chance nodes: probability-weighted average
- Exploitability = BR_value(p0) + BR_value(p1)

25. Implement agents:

```cpp
class CFRAgent {
public:
    CFRAgent(const InfoSetTable& strategy_sum_table);
    std::string act(const LeducState& state) const;  // Sample from strategy
};

class RandomAgent {
    std::string act(const LeducState& state) const;  // Uniform over legal actions
};
```

26. Implement `struct MatchResult` and match player:

```cpp
struct MatchResult {
    double player0_winrate;
    int player0_wins, player1_wins;
    std::vector<double> payoffs;
};

MatchResult play_match(Agent& agent_a, Agent& agent_b, int num_games, int seed);
```

---

### Phase 7 — Checkpointing & Serialization (already in Phase 2–6)

27. Checkpointing is integrated into `InfoSetTable::save()` / `load()` and `CFRTrainer::save_checkpoint()`.
28. Use `nlohmann/json` for JSON serialization of `regret_table` and `strategy_sum_table`.
29. Structure: `{"regret_table": {"info_set": {"action": value, ...}, ...}, "strategy_sum_table": {...}}`

---

### Phase 8 — CLI Human Play Interface (`src/play.cpp`)

28. Implement interactive CLI game loop:

```cpp
class CLIGame {
public:
    CLIGame(const LeducGame& game, const CFRAgent& agent);
    void play_game(int human_player);  // 0 or 1
    void play_session(int num_games);

private:
    void display_state(const LeducState& state, int human_player);
    std::string get_human_action();
};
```

29. Implement `src/play.cpp` entry point:

```cpp
int main(int argc, char* argv[]) {
    // Parse: --checkpoint <path> --games <num> --human-player <0|1>
    auto [strategy, config] = load_checkpoint(checkpoint_path);
    LeducGame game;
    CFRAgent agent(strategy);
    CLIGame cli_game(game, agent);
    cli_game.play_session(num_games);
    return 0;
}
```

- Usage: `./cfr_play --checkpoint checkpoints/final.json --games 10 --human-player 0`

---

### Phase 9 — Visualization & Analytics (`experiments/analyze.py`)

30. Keep analysis in Python for simplicity (matplotlib, numpy for plotting):

- Implement `experiments/analyze.py` to:
  - Read training CSV logs from `data/training_log.csv` (written by C++ trainer)
  - Plot **exploitability vs. iteration**
  - Plot **cumulative regret norm**
  - Plot **win-rate vs. random agent** over checkpoint intervals
  - (Optional) strategy frequency heatmap for key info-sets
- Run: `python experiments/analyze.py --log-file data/training_log.csv`

31. Implement `experiments/train.sh`:

```bash
#!/bin/bash
cd "$(dirname "$0")/.."
mkdir -p checkpoints data
./build/cfr_train --iterations 100000 --output checkpoints/
python experiments/analyze.py --log-file data/training_log.csv
```

---

### Phase 10 — Documentation & README

32. Rewrite `README.md`:

- Project overview: C++17 Leduc CFR solver, no external dependencies
- Build instructions: `cmake --build build`, requires CMake 3.15+, C++17 compiler
- Quick start:
  ```bash
  mkdir build && cd build
  cmake .. && make
  ./cfr_train --iterations 100000 --output ../checkpoints/
  ./cfr_play --checkpoint ../checkpoints/final.json --games 5
  ```
- Architecture overview (ASCII diagram showing Phases 1–8)
- Experimental results placeholder

33. Add doxygen-style docstrings to all public classes and functions in headers.

34. Update `notes.md` as engineering decision log with session timestamps.

---

### Stubs Preserved for Future Extension

- `include/leduc/deep_cfr.h`: stub with class docstring explaining role in Deep CFR (neural network approximation of value function).
- `include/leduc/replay_buffer.h`: stub for MCCFR / Deep CFR experience replay.
- `include/leduc/web_interface.h`: stub for future web-based UI (REST API or WebSocket).

---

### Verification

- **Unit tests**: `./build/cfr_tests` — covers game engine, CFR updates, strategy normalization, exploitability, agent legality.
- **Convergence check**: run 100,000 iterations; expected exploitability < 0.05 mBB/hand (Leduc converges fast).
- **Smoke test**: `./cfr_play --checkpoint checkpoints/100k.json --games 1` — play one hand manually.
- **Match test**: `./cfr_train --iterations 10000 && python experiments/analyze.py` — write logs, analyze convergence.
- **Build test**: `cmake --build build` with `-Wall -Wextra -Werror` (no warnings).

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

- **Language**: C++17 for performance and modern features (structured bindings, `std::optional`, `std::unordered_map`).
- **Build system**: CMake 3.15+ for cross-platform builds.
- **Minimal dependencies**: Only `nlohmann/json` (header-only) for serialization; no external game libraries or ML frameworks.
- **String keys for info-sets**: Tabular CFR uses `"{private}|{public_or_?}|{round0_hist}/{round1_hist}"` — human-readable, debuggable.
- **Immutable `LeducState`**: `apply_action()` returns a new state — correct for recursive CFR.
- **Header-only utilities**: Small helper functions in headers (parsing, constants) to avoid link complexity.
- **Single-threaded first**: Core CFR loops are single-threaded; OpenMP pragmas can be added later without changing interface.
- **Deep CFR stubs**: `deep_cfr.h` and `replay_buffer.h` retained as documented stubs — extensible design.
- **Python for post-analysis**: Training is C++ (performance); analysis (`experiments/analyze.py`) stays Python (quick iteration, plotting).
