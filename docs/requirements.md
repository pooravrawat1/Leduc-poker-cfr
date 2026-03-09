# Requirements: High-Performance Leduc Poker CFR Solver

## 1. Overview

Build a production-grade, highly optimized Leduc Poker solver in C++ that computes Nash Equilibrium strategies using Counterfactual Regret Minimization (CFR). The implementation must prioritize performance, cache efficiency, and avoid common C++ pitfalls like unnecessary heap allocations and slow standard library containers in hot paths.

## 2. Game Rules (Leduc Poker)

### 2.1 Deck Composition
- 6 cards total: 2 Jacks (J), 2 Queens (Q), 2 Kings (K)
- Card encoding: J=0, Q=1, K=2

### 2.2 Game Structure
- 2 players (Player 0 and Player 1)
- 2 betting rounds:
  - Round 0 (Pre-flop): Each player dealt 1 private card
  - Round 1 (Flop): 1 public card revealed
- Ante: 1 chip per player (pot starts at 2)

### 2.3 Betting Rules
- Fixed-limit betting:
  - Round 0: Bet/raise size = 2 chips
  - Round 1: Bet/raise size = 4 chips
- Maximum 2 raises per round (initial bet + 1 raise)
- Actions: Fold, Call/Check, Bet/Raise

### 2.4 Showdown Rules
- Pair (private card matches public card) beats any non-pair
- Between pairs or non-pairs: higher rank wins
- Ties split the pot

## 3. Functional Requirements

### 3.1 Core CFR Solver

**FR-1.1: Information Set Representation**
- The system SHALL represent information sets using compact bit-packed integers (uint16_t or uint32_t)
- The system SHALL encode: player card, public card (or absence), and betting history
- The system SHALL support direct array indexing without hash table lookups

**FR-1.2: CFR Algorithm**
- The system SHALL implement Vanilla CFR with regret matching
- The system SHALL support CFR+ (regret floor at 0) as a configuration option
- The system SHALL compute counterfactual values for all information sets
- The system SHALL accumulate regrets and strategy sums across iterations

**FR-1.3: Strategy Extraction**
- The system SHALL compute average strategy from accumulated strategy sums
- The system SHALL support strategy serialization to disk
- The system SHALL support strategy loading from disk

### 3.2 Performance Requirements

**FR-2.1: Memory Efficiency**
- The system SHALL use fixed-size arrays for information set storage (no dynamic allocation in hot loop)
- The system SHALL use stack allocation for game state traversal
- The system SHALL achieve cache-line alignment for critical data structures
- The system SHALL NOT use std::string, std::map, or std::unordered_map in the CFR recursion

**FR-2.2: Computational Efficiency**
- The system SHALL complete 1 million CFR iterations in under 60 seconds on a modern CPU (single-threaded)
- The system SHALL use branchless code for terminal state evaluation where possible
- The system SHALL minimize function call overhead in the recursion

### 3.3 Validation & Verification

**FR-3.1: Exploitability Calculation**
- The system SHALL compute exploitability (NashConv) of the learned strategy
- The system SHALL compute best-response values for both players
- The system SHALL report exploitability in milli-big-blinds per hand (mbb/hand)

**FR-3.2: Convergence Criteria**
- The system SHALL achieve exploitability < 1 mbb/hand after sufficient iterations
- The system SHALL log exploitability at configurable intervals
- The system SHALL support early stopping when convergence threshold is met

**FR-3.3: Correctness Validation**
- The system SHALL validate that Jack (weakest hand) folds to aggression pre-flop in equilibrium
- The system SHALL validate that King (strongest hand) raises pre-flop in equilibrium
- The system SHALL validate that Queen shows mixed strategy behavior

### 3.4 Usability Requirements

**FR-4.1: Training Interface**
- The system SHALL provide a command-line training executable
- The system SHALL support configurable iteration count
- The system SHALL support configurable checkpoint intervals
- The system SHALL log training progress (iteration, exploitability, time)

**FR-4.2: Strategy Analysis**
- The system SHALL export strategy to human-readable format
- The system SHALL support querying strategy for specific information sets
- The system SHALL display action probabilities for key decision points

**FR-4.3: Testing Interface**
- The system SHALL provide unit tests for game logic
- The system SHALL provide unit tests for CFR updates
- The system SHALL provide integration tests for convergence

## 4. Non-Functional Requirements

### 4.1 Code Quality

**NFR-1.1: Modern C++ Standards**
- The system SHALL use C++17 or later
- The system SHALL compile with -Wall -Wextra -Werror with zero warnings
- The system SHALL use constexpr for compile-time constants

**NFR-1.2: Build System**
- The system SHALL use CMake 3.15+
- The system SHALL support Release and Debug build configurations
- The system SHALL enable optimization flags (-O3, -march=native) in Release mode

### 4.2 Portability

**NFR-2.1: Platform Support**
- The system SHALL compile on Linux (GCC 9+, Clang 10+)
- The system SHALL compile on macOS (Apple Clang 12+)
- The system SHALL compile on Windows (MSVC 2019+, MinGW)

### 4.3 Dependencies

**NFR-3.1: Minimal Dependencies**
- The system SHALL have ZERO external dependencies for core CFR solver
- The system MAY use standard library containers for non-performance-critical code
- The system MAY use header-only libraries for serialization (optional)

## 5. Acceptance Criteria

### 5.1 Performance Benchmarks

**AC-1.1: Iteration Speed**
- GIVEN a modern CPU (e.g., Intel i7 or AMD Ryzen)
- WHEN running 1 million CFR iterations
- THEN the solver completes in under 60 seconds (single-threaded)

**AC-1.2: Memory Footprint**
- GIVEN the complete information set storage
- WHEN all ~936 information sets are allocated
- THEN total memory usage is under 100 KB

### 5.2 Correctness Validation

**AC-2.1: Exploitability Convergence**
- GIVEN 1 million CFR iterations
- WHEN exploitability is computed
- THEN exploitability is less than 1 mbb/hand

**AC-2.2: Jack Pre-Flop Strategy**
- GIVEN a solved strategy
- WHEN Player 0 holds Jack and faces a bet
- THEN fold probability is > 80%

**AC-2.3: King Pre-Flop Strategy**
- GIVEN a solved strategy
- WHEN Player 0 holds King as first to act
- THEN bet probability is > 70%

### 5.3 Code Quality

**AC-3.1: Compilation**
- GIVEN the complete codebase
- WHEN compiled with -Wall -Wextra -Werror
- THEN zero warnings are produced

**AC-3.2: Unit Tests**
- GIVEN the test suite
- WHEN all tests are executed
- THEN 100% of tests pass

**AC-3.3: No Dynamic Allocation in Hot Loop**
- GIVEN the CFR recursion function
- WHEN profiled with valgrind or similar
- THEN zero heap allocations occur during tree traversal

## 6. Out of Scope

The following are explicitly OUT OF SCOPE for this implementation:

- Multi-threading or parallel CFR (future extension)
- SIMD vectorization (future extension)
- Deep CFR or neural network approximation
- Monte Carlo CFR (MCCFR) sampling variants
- GUI or web interface
- Multi-player variants (>2 players)
- Other poker variants (Texas Hold'em, Omaha, etc.)

## 7. Success Metrics

### 7.1 Primary Metrics
- Exploitability < 1 mbb/hand after 1M iterations
- Iteration throughput > 16,000 iterations/second
- Memory footprint < 100 KB

### 7.2 Secondary Metrics
- Code coverage > 90% for core modules
- Zero memory leaks (valgrind clean)
- Compilation time < 10 seconds (clean build)

## 8. Glossary

- **CFR**: Counterfactual Regret Minimization
- **Information Set**: All game states indistinguishable to a player
- **Regret**: Difference between action value and node value
- **Exploitability**: Sum of best-response values against the strategy
- **mbb/hand**: Milli-big-blinds per hand (1/1000 of a big blind)
- **Nash Equilibrium**: Strategy profile where no player can improve by deviating
