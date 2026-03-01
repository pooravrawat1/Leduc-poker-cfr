# Tasks: High-Performance Leduc Poker CFR Solver

## Phase 1: Foundation & Data Structures

### 1.1 Project Setup
- [x] 1.1.1 Create CMakeLists.txt with C++17 configuration
- [x] 1.1.2 Set up directory structure (include/, src/, tests/, benchmarks/)
- [x] 1.1.3 Configure compiler flags (-Wall -Wextra -Werror -O3 -march=native)
- [x] 1.1.4 Create .gitignore for build artifacts
- [x] 1.1.5 Create README.md with build instructions

### 1.2 Core Type Definitions
- [ ] 1.2.1 Create include/leduc/types.h with basic types and constants
  - Define Action enum (FOLD, CALL, RAISE)
  - Define card encoding constants (J=0, Q=1, K=2)
  - Define bet sizes and game constants
- [ ] 1.2.2 Create GameState struct in include/leduc/game_state.h
  - 16-byte compact representation
  - uint8_t fields for cards, round, player, pot, etc.
- [ ] 1.2.3 Create CFRNode struct in include/leduc/cfr_node.h
  - 64-byte cache-aligned structure
  - regret_sum, strategy_sum, strategy arrays
  - Metadata fields

### 1.3 Information Set Encoding
- [ ] 1.3.1 Design 16-bit information set encoding scheme
  - Document bit layout in include/leduc/infoset.h
  - Player card (2 bits), public card (2 bits), betting history (6 bits)
- [ ] 1.3.2 Implement compute_infoset_key() function
  - Takes GameState and player, returns uint16_t
- [ ] 1.3.3 Implement decode_infoset_key() function (for debugging)
- [ ] 1.3.4 Write unit tests for information set encoding
  - Test uniqueness for different game states
  - Test that equivalent states produce same key
  - Test all ~936 possible information sets

## Phase 2: Game Engine

### 2.1 Game State Operations
- [ ] 2.1.1 Implement is_terminal() function
  - Check for fold or showdown conditions
- [ ] 2.1.2 Implement is_chance_node() function
  - Check if public card needs to be dealt
- [ ] 2.1.3 Implement legal_actions_mask() function
  - Branchless computation of legal action bitmask
- [ ] 2.1.4 Implement count_legal_actions() function
  - Count bits in action mask

### 2.2 State Transitions
- [ ] 2.2.1 Implement apply_action() function
  - Handle FOLD: set folded player
  - Handle CALL: update pot, advance round/player
  - Handle RAISE: update pot, to_call, raises_this_round
  - Update action_history bitfield
- [ ] 2.2.2 Implement apply_chance() function
  - Deal public card at start of round 1
- [ ] 2.2.3 Write unit tests for state transitions
  - Test all action types
  - Test round advancement
  - Test terminal state detection

### 2.3 Terminal Evaluation
- [ ] 2.3.1 Implement evaluate_showdown() function (branchless)
  - Compare pairs vs non-pairs
  - Compare card ranks
  - Return winner (-1, 0, 1)
- [ ] 2.3.2 Implement terminal_utility() function
  - Handle fold case
  - Handle showdown case
  - Return utility for specified player
- [ ] 2.3.3 Write unit tests for terminal evaluation
  - Test pair vs non-pair
  - Test pair vs pair (higher rank wins)
  - Test non-pair vs non-pair
  - Test ties (split pot)
  - Test fold scenarios

## Phase 3: CFR Algorithm Core

### 3.1 Regret Matching
- [ ] 3.1.1 Implement regret_match() function
  - Clamp negative regrets to 0
  - Normalize positive regrets
  - Return uniform strategy if all regrets ≤ 0
- [ ] 3.1.2 Implement get_average_strategy() function
  - Normalize strategy_sum to get average strategy
  - Handle case where strategy_sum is all zeros
- [ ] 3.1.3 Write unit tests for regret matching
  - Test with positive regrets
  - Test with negative regrets
  - Test with mixed regrets
  - Test normalization

### 3.2 Node Storage
- [ ] 3.2.1 Create global node array (CFRNode g_nodes[MAX_INFOSETS])
- [ ] 3.2.2 Implement initialize_nodes() function
  - Zero-initialize all nodes
  - Set num_actions for each information set
- [ ] 3.2.3 Implement get_node() inline function
  - Direct array indexing by infoset_key

### 3.3 CFR Recursion
- [ ] 3.3.1 Implement cfr() recursive function signature
  - Parameters: GameState, player, reach_p0, reach_p1
  - Returns: float (counterfactual value)
- [ ] 3.3.2 Implement terminal node case
  - Return terminal_utility()
- [ ] 3.3.3 Implement chance node case
  - Enumerate possible public cards
  - Recurse with probability weighting
  - Return expected value
- [ ] 3.3.4 Implement decision node case
  - Get current strategy from regret matching
  - Recurse for each action
  - Compute action values and node value
  - Update regrets (for acting player)
  - Update strategy sums (for acting player)
  - Return node value
- [ ] 3.3.5 Write unit tests for CFR recursion
  - Test terminal node returns correct utility
  - Test chance node averages correctly
  - Test regret updates are correct
  - Test strategy sum updates are correct

### 3.4 Training Loop
- [ ] 3.4.1 Implement train_iteration() function
  - Enumerate all initial card deals
  - Call cfr() for player 0 on all deals
  - Call cfr() for player 1 on all deals
- [ ] 3.4.2 Implement train() function with iteration loop
  - Call train_iteration() N times
  - Log progress every K iterations
- [ ] 3.4.3 Write integration test for training
  - Run 1000 iterations
  - Verify regrets and strategy sums are updated

## Phase 4: Exploitability Calculation

### 4.1 Best-Response Calculation
- [ ] 4.1.1 Implement br_recursive() function
  - Terminal case: return utility
  - Chance case: average over outcomes
  - BR player case: take MAX over actions
  - Opponent case: follow average strategy
- [ ] 4.1.2 Implement best_response_value() function
  - Enumerate all initial deals
  - Call br_recursive() for each deal
  - Return average value
- [ ] 4.1.3 Write unit tests for best-response
  - Test against known strategies
  - Test against uniform random strategy
  - Test against always-fold strategy

### 4.2 Exploitability Computation
- [ ] 4.2.1 Implement compute_exploitability() function
  - Compute BR value for player 0
  - Compute BR value for player 1
  - Return sum (Nash distance)
- [ ] 4.2.2 Write unit tests for exploitability
  - Test that exploitability ≥ 0
  - Test that exploitability decreases with training
- [ ] 4.2.3 Implement exploitability logging
  - Log exploitability every N iterations
  - Convert to milli-big-blinds per hand

## Phase 5: Strategy Analysis & Validation

### 5.1 Strategy Extraction
- [ ] 5.1.1 Implement get_strategy_for_infoset() function
  - Return average strategy for given infoset key
- [ ] 5.1.2 Implement print_strategy() function
  - Print human-readable strategy for all infosets
  - Format: "J|?|: Fold=0.85, Call=0.15, Raise=0.00"
- [ ] 5.1.3 Implement save_strategy() function (optional)
  - Binary serialization of node array
- [ ] 5.1.4 Implement load_strategy() function (optional)
  - Binary deserialization

### 5.2 Equilibrium Validation
- [ ] 5.2.1 Implement validate_jack_strategy() function
  - Check Jack folds to bet with probability > 0.8
- [ ] 5.2.2 Implement validate_king_strategy() function
  - Check King bets first-to-act with probability > 0.7
- [ ] 5.2.3 Implement validate_queen_strategy() function
  - Check Queen shows mixed behavior
- [ ] 5.2.4 Write integration test for equilibrium validation
  - Train for 100k iterations
  - Run all validation checks
  - Assert exploitability < 1 mbb/hand

## Phase 6: Main Executable & CLI

### 6.1 Command-Line Interface
- [ ] 6.1.1 Implement parse_args() function
  - Parse --iterations, --log-interval, --checkpoint-interval
- [ ] 6.1.2 Implement main() function in src/main.cpp
  - Initialize nodes
  - Run training loop
  - Log progress and exploitability
  - Print final strategy
- [ ] 6.1.3 Add timing and performance metrics
  - Measure iterations per second
  - Report total training time

### 6.2 Output & Logging
- [ ] 6.2.1 Implement progress logging
  - Print iteration number, exploitability, time
- [ ] 6.2.2 Implement final strategy output
  - Print key information sets and strategies
  - Highlight Jack, Queen, King pre-flop strategies
- [ ] 6.2.3 Add CSV export option (optional)
  - Export iteration, exploitability, time to CSV

## Phase 7: Testing & Validation

### 7.1 Unit Test Suite
- [ ] 7.1.1 Create test_main.cpp with test runner
- [ ] 7.1.2 Ensure all unit tests pass
- [ ] 7.1.3 Measure code coverage (aim for >90%)

### 7.2 Integration Tests
- [ ] 7.2.1 Test convergence to Nash equilibrium
  - Run 100k iterations
  - Verify exploitability < 1 mbb/hand
- [ ] 7.2.2 Test strategy symmetry
  - Verify player 0 and player 1 strategies are symmetric
- [ ] 7.2.3 Test zero-sum property
  - Verify sum of utilities equals zero at all terminal states

### 7.3 Performance Benchmarks
- [ ] 7.3.1 Benchmark CFR iteration speed
  - Measure iterations per second
  - Target: >16,000 iterations/second
- [ ] 7.3.2 Benchmark memory usage
  - Verify total memory < 100 KB
- [ ] 7.3.3 Profile with perf/valgrind
  - Identify hotspots
  - Verify no heap allocations in CFR recursion
  - Check cache miss rate

## Phase 8: Optimization & Polish

### 8.1 Performance Optimization
- [ ] 8.1.1 Profile with perf and identify bottlenecks
- [ ] 8.1.2 Optimize hot paths (CFR recursion, regret matching)
- [ ] 8.1.3 Add compiler hints (__builtin_expect, likely/unlikely)
- [ ] 8.1.4 Verify branchless code in terminal evaluation

### 8.2 Code Quality
- [ ] 8.2.1 Add doxygen-style comments to all public functions
- [ ] 8.2.2 Run clang-format on all source files
- [ ] 8.2.3 Run clang-tidy and fix warnings
- [ ] 8.2.4 Verify zero compiler warnings with -Wall -Wextra -Werror

### 8.3 Documentation
- [ ] 8.3.1 Write comprehensive README.md
  - Project overview
  - Build instructions
  - Usage examples
  - Performance benchmarks
- [ ] 8.3.2 Document information set encoding scheme
- [ ] 8.3.3 Document expected equilibrium strategies
- [ ] 8.3.4 Add inline code comments for complex logic

## Phase 9: Advanced Features (Optional)

### 9.1 CFR+ Variant
- [ ]* 9.1.1 Add CFR+ support (regret floor at 0)
- [ ]* 9.1.2 Add compile-time flag to enable CFR+
- [ ]* 9.1.3 Benchmark CFR+ vs Vanilla CFR convergence

### 9.2 Linear CFR
- [ ]* 9.2.1 Implement linear weighting of regrets
- [ ]* 9.2.2 Compare convergence speed to Vanilla CFR

### 9.3 Discounted CFR
- [ ]* 9.3.1 Implement regret discounting
- [ ]* 9.3.2 Add discount factor as parameter

### 9.4 Multi-Threading
- [ ]* 9.4.1 Add OpenMP pragmas for parallel tree traversal
- [ ]* 9.4.2 Benchmark single-threaded vs multi-threaded
- [ ]* 9.4.3 Ensure thread-safe regret updates

### 9.5 SIMD Optimization
- [ ]* 9.5.1 Vectorize regret matching with AVX/SSE
- [ ]* 9.5.2 Vectorize strategy sum updates
- [ ]* 9.5.3 Benchmark SIMD vs scalar implementation

## Phase 10: Deployment & Release

### 10.1 Build Artifacts
- [ ] 10.1.1 Create release build script
- [ ] 10.1.2 Generate optimized binary
- [ ] 10.1.3 Strip debug symbols for release

### 10.2 Documentation
- [ ] 10.2.1 Write CHANGELOG.md
- [ ] 10.2.2 Write CONTRIBUTING.md (if open-source)
- [ ] 10.2.3 Add LICENSE file

### 10.3 Release
- [ ] 10.3.1 Tag release version
- [ ] 10.3.2 Create GitHub release with binary
- [ ] 10.3.3 Publish performance benchmarks

---

## Task Dependencies

```
Phase 1 (Foundation)
  └─> Phase 2 (Game Engine)
       └─> Phase 3 (CFR Core)
            ├─> Phase 4 (Exploitability)
            └─> Phase 5 (Strategy Analysis)
                 └─> Phase 6 (Main Executable)
                      └─> Phase 7 (Testing)
                           └─> Phase 8 (Optimization)
                                └─> Phase 9 (Advanced Features)*
                                     └─> Phase 10 (Release)
```

## Estimated Timeline

- Phase 1: 2-3 hours
- Phase 2: 3-4 hours
- Phase 3: 4-6 hours
- Phase 4: 2-3 hours
- Phase 5: 2-3 hours
- Phase 6: 1-2 hours
- Phase 7: 3-4 hours
- Phase 8: 2-3 hours
- Phase 9: 4-6 hours (optional)
- Phase 10: 1-2 hours

**Total: 24-36 hours (core features), 28-42 hours (with optional features)**

## Success Criteria

- [ ] All unit tests pass
- [ ] Exploitability < 1 mbb/hand after 100k iterations
- [ ] Iteration speed > 16,000 iterations/second
- [ ] Memory usage < 100 KB
- [ ] Zero compiler warnings
- [ ] Zero heap allocations in CFR recursion
- [ ] Jack folds to bet with probability > 0.8
- [ ] King bets first-to-act with probability > 0.7
