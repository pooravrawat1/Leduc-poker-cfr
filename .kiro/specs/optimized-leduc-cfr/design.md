# Design: High-Performance Leduc Poker CFR Solver

## 1. Architecture Overview

### 1.1 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     CFR Solver System                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐      ┌──────────────┐                    │
│  │   Training   │─────▶│  CFR Engine  │                    │
│  │   Harness    │      │              │                    │
│  └──────────────┘      └──────┬───────┘                    │
│                               │                             │
│                               ▼                             │
│                    ┌──────────────────┐                    │
│                    │  InfoSet Storage │                    │
│                    │  (Fixed Arrays)  │                    │
│                    └──────────────────┘                    │
│                               │                             │
│                               ▼                             │
│  ┌──────────────┐      ┌──────────────┐                    │
│  │ Exploitability│◀────│ Game Engine  │                    │
│  │  Calculator  │      │  (Branchless)│                    │
│  └──────────────┘      └──────────────┘                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Module Hierarchy

1. **Game Engine** (leduc_game.h): State representation, action generation, terminal evaluation
2. **InfoSet Encoding** (infoset.h): Bit-packed information set keys and indexing
3. **CFR Core** (cfr_solver.h): Regret minimization algorithm
4. **Storage** (cfr_tables.h): Fixed-size arrays for regrets and strategies
5. **Evaluation** (exploitability.h): Best-response and Nash distance calculation
6. **Training** (train.cpp): Main training loop and logging

## 2. Data Representation & Memory Layout

### 2.1 Information Set Encoding (16-bit)

Leduc Poker has approximately 936 unique information sets. We use a 16-bit encoding:

```
Bit Layout (uint16_t):
┌─────────────────────────────────────────────────────────────┐
│ 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0           │
├──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┤           │
│ P│ P│ B│ B│ B│ B│ B│ B│ B│ B│ C│ C│ A│ A│ A│ A│           │
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘           │

Bits 0-3   (4 bits): Action sequence encoding (16 possible sequences)
Bits 4-5   (2 bits): Player card (0=J, 1=Q, 2=K, 3=unused)
Bits 6-7   (2 bits): Public card (0=J, 1=Q, 2=K, 3=none/pre-flop)
Bits 8-13  (6 bits): Betting history encoding (64 possible histories)
Bits 14-15 (2 bits): Round (0=pre-flop, 1=flop, 2-3=unused)
```

**Action Sequence Encoding (4 bits):**
```
0000 = (empty - first to act)
0001 = Check
0010 = Bet
0011 = Check-Check
0100 = Check-Bet
0101 = Check-Bet-Call
0110 = Check-Bet-Raise
0111 = Bet-Call
1000 = Bet-Raise
1001 = Bet-Raise-Call
... (up to 16 sequences)
```

**Betting History Encoding (6 bits):**
- Bits 0-2: Round 0 actions (3 bits = 8 sequences)
- Bits 3-5: Round 1 actions (3 bits = 8 sequences)

### 2.2 Node Storage Structure

```cpp
// Cache-aligned node structure (64 bytes = 1 cache line)
struct alignas(64) CFRNode {
    // Regret sums for each action (3 actions max: Fold, Call, Raise)
    float regret_sum[3];      // 12 bytes
    
    // Strategy sums for each action
    float strategy_sum[3];    // 12 bytes
    
    // Current strategy (computed from regrets)
    float strategy[3];        // 12 bytes
    
    // Metadata
    uint8_t num_actions;      // 1 byte (1-3 actions)
    uint8_t action_mask;      // 1 byte (bit mask of legal actions)
    uint16_t infoset_key;     // 2 bytes
    
    // Padding to 64 bytes
    uint8_t padding[24];      // 24 bytes
};
```

### 2.3 Fixed-Size Storage Arrays

```cpp
// Maximum possible information sets (conservative estimate)
constexpr size_t MAX_INFOSETS = 1024;

// Global storage (stack or static allocation)
CFRNode g_nodes[MAX_INFOSETS];

// Direct array indexing (no hash table)
inline CFRNode& get_node(uint16_t infoset_key) {
    return g_nodes[infoset_key];
}
```

### 2.4 Game State Representation

```cpp
// Compact game state (fits in 16 bytes)
struct GameState {
    uint8_t cards[3];           // [p0_card, p1_card, public_card]
    uint8_t round;              // 0 or 1
    uint8_t player;             // 0 or 1
    uint8_t pot;                // Current pot size
    uint8_t to_call;            // Chips needed to call
    uint8_t raises_this_round;  // 0, 1, or 2
    uint8_t action_history;     // Bit-packed action sequence
    uint8_t folded;             // 0xFF = no fold, 0/1 = player who folded
    uint8_t padding[7];         // Align to 16 bytes
};
```

## 3. Game Engine Design

### 3.1 Action Enumeration

```cpp
enum class Action : uint8_t {
    FOLD  = 0,
    CALL  = 1,  // Also CHECK when to_call == 0
    RAISE = 2   // Also BET when to_call == 0
};

// Branchless action legality check
inline uint8_t legal_actions_mask(const GameState& state) {
    // FOLD always legal except when to_call == 0
    // CALL always legal
    // RAISE legal if raises_this_round < 2
    uint8_t mask = 0;
    mask |= (state.to_call > 0) << 0;  // FOLD
    mask |= 1 << 1;                     // CALL
    mask |= (state.raises_this_round < 2) << 2;  // RAISE
    return mask;
}
```

### 3.2 Terminal State Evaluation (Branchless)

```cpp
// Branchless showdown evaluation
inline int evaluate_showdown(uint8_t p0_card, uint8_t p1_card, uint8_t public_card) {
    // Check for pairs
    bool p0_pair = (p0_card == public_card);
    bool p1_pair = (p1_card == public_card);
    
    // Pair comparison
    int pair_cmp = int(p0_pair) - int(p1_pair);
    
    // Card rank comparison
    int rank_cmp = int(p0_card) - int(p1_card);
    
    // Branchless selection: pair_cmp if either has pair, else rank_cmp
    int result = pair_cmp * (p0_pair | p1_pair) + rank_cmp * !(p0_pair | p1_pair);
    
    // Return: 1 if p0 wins, -1 if p1 wins, 0 if tie
    return (result > 0) - (result < 0);
}

// Terminal utility calculation
inline float terminal_utility(const GameState& state, uint8_t player) {
    if (state.folded != 0xFF) {
        // Someone folded
        int winner = 1 - state.folded;
        float pot_value = state.pot * 0.5f;
        return (winner == player) ? pot_value : -pot_value;
    }
    
    // Showdown
    int outcome = evaluate_showdown(state.cards[0], state.cards[1], state.cards[2]);
    
    // Convert outcome to utility for player
    float pot_value = state.pot * 0.5f;
    int sign = (player == 0) ? outcome : -outcome;
    return pot_value * sign;
}
```

### 3.3 State Transition

```cpp
// Apply action (returns new state, no heap allocation)
inline GameState apply_action(const GameState& state, Action action) {
    GameState next = state;
    
    switch (action) {
        case Action::FOLD:
            next.folded = state.player;
            break;
            
        case Action::CALL:
            next.pot += state.to_call;
            next.to_call = 0;
            
            // Advance round or player
            if (state.action_history & 0x01) {  // Someone acted before
                if (state.round == 0) {
                    next.round = 1;
                    next.player = 0;  // Reset to first player
                } else {
                    // Terminal (both called in round 1)
                }
            } else {
                next.player = 1 - state.player;
            }
            break;
            
        case Action::RAISE:
            {
                uint8_t raise_size = (state.round == 0) ? 2 : 4;
                next.pot += state.to_call + raise_size;
                next.to_call = raise_size;
                next.raises_this_round++;
                next.player = 1 - state.player;
            }
            break;
    }
    
    // Update action history
    next.action_history = (state.action_history << 2) | uint8_t(action);
    
    return next;
}
```

## 4. CFR Algorithm Design

### 4.1 Algorithm Selection: Vanilla CFR

**Decision: Use Vanilla CFR (not CS-CFR or CFR+)**

Rationale:
- Leduc has only ~936 information sets (tiny game tree)
- Vanilla CFR traverses entire tree every iteration (no sampling)
- Full traversal is fast (<1ms per iteration) and guarantees convergence
- CS-CFR adds sampling variance without performance benefit for small games
- CFR+ can be added as optional optimization (regret floor at 0)

### 4.2 CFR Recursion Function Signature

```cpp
// Returns counterfactual value for the specified player
float cfr(
    const GameState& state,
    uint8_t player,           // Player computing counterfactual value for
    float reach_p0,           // Reach probability for player 0
    float reach_p1            // Reach probability for player 1
);
```

### 4.3 CFR Recursion Pseudocode

```cpp
float cfr(const GameState& state, uint8_t player, float reach_p0, float reach_p1) {
    // Base case: terminal node
    if (is_terminal(state)) {
        return terminal_utility(state, player);
    }
    
    // Chance node: deal public card (only at start of round 1)
    if (is_chance_node(state)) {
        float value = 0.0f;
        for (uint8_t card = 0; card < 3; card++) {
            if (card != state.cards[0] && card != state.cards[1]) {
                GameState next = state;
                next.cards[2] = card;
                float prob = 1.0f / (6 - 2);  // 4 remaining cards, 2 of each rank
                value += prob * cfr(next, player, reach_p0, reach_p1);
            }
        }
        return value;
    }
    
    // Decision node
    uint8_t acting_player = state.player;
    uint16_t infoset_key = compute_infoset_key(state, acting_player);
    CFRNode& node = g_nodes[infoset_key];
    
    // Get current strategy from regret matching
    float strategy[3];
    regret_match(node.regret_sum, node.num_actions, strategy);
    
    // Compute counterfactual values for each action
    float action_values[3] = {0.0f};
    float node_value = 0.0f;
    
    for (uint8_t a = 0; a < node.num_actions; a++) {
        GameState next = apply_action(state, Action(a));
        
        // Recurse with updated reach probabilities
        if (acting_player == 0) {
            action_values[a] = cfr(next, player, reach_p0 * strategy[a], reach_p1);
        } else {
            action_values[a] = cfr(next, player, reach_p0, reach_p1 * strategy[a]);
        }
        
        node_value += strategy[a] * action_values[a];
    }
    
    // Update regrets and strategy sum (only for acting player)
    if (acting_player == player) {
        float opponent_reach = (player == 0) ? reach_p1 : reach_p0;
        float player_reach = (player == 0) ? reach_p0 : reach_p1;
        
        for (uint8_t a = 0; a < node.num_actions; a++) {
            float regret = action_values[a] - node_value;
            node.regret_sum[a] += opponent_reach * regret;
            node.strategy_sum[a] += player_reach * strategy[a];
        }
    }
    
    return node_value;
}
```

### 4.4 Regret Matching

```cpp
// Compute current strategy from regret sums
inline void regret_match(const float* regret_sum, uint8_t num_actions, float* strategy) {
    float sum_positive_regret = 0.0f;
    
    // Sum positive regrets
    for (uint8_t a = 0; a < num_actions; a++) {
        float positive_regret = std::max(0.0f, regret_sum[a]);
        strategy[a] = positive_regret;
        sum_positive_regret += positive_regret;
    }
    
    // Normalize (or uniform if all regrets non-positive)
    if (sum_positive_regret > 0.0f) {
        float inv_sum = 1.0f / sum_positive_regret;
        for (uint8_t a = 0; a < num_actions; a++) {
            strategy[a] *= inv_sum;
        }
    } else {
        float uniform = 1.0f / num_actions;
        for (uint8_t a = 0; a < num_actions; a++) {
            strategy[a] = uniform;
        }
    }
}
```

### 4.5 Training Iteration

```cpp
void train_iteration() {
    // Traverse tree for player 0
    for (uint8_t p0_card = 0; p0_card < 3; p0_card++) {
        for (uint8_t p1_card = 0; p1_card < 3; p1_card++) {
            if (p0_card == p1_card) continue;  // Can't deal same card
            
            GameState initial;
            initial.cards[0] = p0_card;
            initial.cards[1] = p1_card;
            initial.cards[2] = 0xFF;  // No public card yet
            initial.round = 0;
            initial.player = 0;
            initial.pot = 2;  // Antes
            initial.to_call = 0;
            initial.raises_this_round = 0;
            initial.action_history = 0;
            initial.folded = 0xFF;
            
            float prob = 1.0f / 6.0f;  // Probability of this deal
            cfr(initial, 0, prob, prob);
        }
    }
    
    // Traverse tree for player 1
    for (uint8_t p0_card = 0; p0_card < 3; p0_card++) {
        for (uint8_t p1_card = 0; p1_card < 3; p1_card++) {
            if (p0_card == p1_card) continue;
            
            GameState initial;
            // ... (same setup)
            
            float prob = 1.0f / 6.0f;
            cfr(initial, 1, prob, prob);
        }
    }
}
```

## 5. Exploitability Calculation

### 5.1 Mathematical Foundation

**Exploitability** (Nash Distance) measures how far a strategy is from Nash Equilibrium:

```
Exploitability(σ) = BR₀(σ₁) + BR₁(σ₀)
```

Where:
- `σ = (σ₀, σ₁)` is the strategy profile
- `BR_i(σ_{-i})` is the best-response value for player i against opponent's strategy

At Nash Equilibrium: `Exploitability(σ*) = 0`

### 5.2 Best-Response Calculation

```cpp
// Compute best-response value for player against opponent's average strategy
float best_response_value(uint8_t player) {
    float total_value = 0.0f;
    int num_deals = 0;
    
    // Enumerate all possible deals
    for (uint8_t p0_card = 0; p0_card < 3; p0_card++) {
        for (uint8_t p1_card = 0; p1_card < 3; p1_card++) {
            if (p0_card == p1_card) continue;
            
            GameState initial;
            // ... (setup initial state)
            
            float value = br_recursive(initial, player);
            total_value += value;
            num_deals++;
        }
    }
    
    return total_value / num_deals;
}

// Recursive best-response calculation
float br_recursive(const GameState& state, uint8_t br_player) {
    if (is_terminal(state)) {
        return terminal_utility(state, br_player);
    }
    
    if (is_chance_node(state)) {
        // Average over chance outcomes
        float value = 0.0f;
        for (uint8_t card = 0; card < 3; card++) {
            if (card != state.cards[0] && card != state.cards[1]) {
                GameState next = state;
                next.cards[2] = card;
                float prob = 0.25f;  // 1/4 (4 remaining cards)
                value += prob * br_recursive(next, br_player);
            }
        }
        return value;
    }
    
    uint8_t acting_player = state.player;
    
    if (acting_player == br_player) {
        // Best-response player: take MAX over actions
        float max_value = -1e9f;
        uint8_t num_actions = count_legal_actions(state);
        
        for (uint8_t a = 0; a < num_actions; a++) {
            GameState next = apply_action(state, Action(a));
            float value = br_recursive(next, br_player);
            max_value = std::max(max_value, value);
        }
        
        return max_value;
    } else {
        // Opponent: follow average strategy
        uint16_t infoset_key = compute_infoset_key(state, acting_player);
        CFRNode& node = g_nodes[infoset_key];
        
        // Get average strategy
        float avg_strategy[3];
        get_average_strategy(node, avg_strategy);
        
        float value = 0.0f;
        for (uint8_t a = 0; a < node.num_actions; a++) {
            GameState next = apply_action(state, Action(a));
            value += avg_strategy[a] * br_recursive(next, br_player);
        }
        
        return value;
    }
}

// Compute exploitability
float compute_exploitability() {
    float br0 = best_response_value(0);
    float br1 = best_response_value(1);
    return br0 + br1;
}
```

### 5.3 Expected Equilibrium Behavior

**Jack (Weakest Hand) Pre-Flop:**
- First to act: Check ~90%, Bet ~10% (bluff)
- Facing bet: Fold ~85-95%, Call ~5-15%

**Queen (Middle Hand) Pre-Flop:**
- First to act: Check ~60%, Bet ~40%
- Facing bet: Call ~70%, Fold ~20%, Raise ~10%

**King (Strongest Hand) Pre-Flop:**
- First to act: Bet ~75-85%, Check ~15-25% (trap)
- Facing bet: Raise ~80%, Call ~20%

**Sanity Check:**
```cpp
// Validate Jack folds to aggression
void validate_equilibrium() {
    // Jack facing bet pre-flop
    GameState state;
    state.cards[0] = 0;  // Jack
    state.round = 0;
    state.to_call = 2;   // Facing bet
    
    uint16_t infoset_key = compute_infoset_key(state, 0);
    CFRNode& node = g_nodes[infoset_key];
    
    float avg_strategy[3];
    get_average_strategy(node, avg_strategy);
    
    // avg_strategy[0] should be > 0.8 (fold probability)
    assert(avg_strategy[0] > 0.8f && "Jack should fold to bet");
}
```

## 6. Project Structure

```
leduc-cfr/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/
│   └── leduc/
│       ├── types.h              # Basic types and constants
│       ├── game_state.h         # GameState struct and operations
│       ├── infoset.h            # Information set encoding
│       ├── cfr_node.h           # CFRNode structure
│       ├── cfr_solver.h         # Main CFR algorithm
│       ├── exploitability.h     # Best-response calculation
│       └── strategy.h           # Strategy extraction and serialization
│
├── src/
│   ├── game_state.cpp           # Game logic implementation
│   ├── infoset.cpp              # InfoSet encoding/decoding
│   ├── cfr_solver.cpp           # CFR recursion
│   ├── exploitability.cpp       # Exploitability calculation
│   ├── strategy.cpp             # Strategy utilities
│   └── main.cpp                 # Training executable
│
├── tests/
│   ├── test_game_state.cpp      # Game logic tests
│   ├── test_infoset.cpp         # InfoSet encoding tests
│   ├── test_cfr.cpp             # CFR algorithm tests
│   ├── test_exploitability.cpp  # Exploitability tests
│   └── test_main.cpp            # Test runner
│
├── benchmarks/
│   ├── bench_cfr.cpp            # CFR iteration speed
│   └── bench_game.cpp           # Game state operations
│
└── scripts/
    ├── build.sh                 # Build script
    ├── run_tests.sh             # Test runner
    └── analyze_strategy.py      # Strategy analysis (optional)
```

## 7. Build Configuration

### 7.1 CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.15)
project(leduc-cfr CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/game_state.cpp
    src/infoset.cpp
    src/cfr_solver.cpp
    src/exploitability.cpp
    src/strategy.cpp
)

# Main executable
add_executable(leduc-cfr src/main.cpp ${SOURCES})

# Test executable
add_executable(leduc-tests
    tests/test_main.cpp
    tests/test_game_state.cpp
    tests/test_infoset.cpp
    tests/test_cfr.cpp
    tests/test_exploitability.cpp
    ${SOURCES}
)

# Benchmark executable (optional)
add_executable(leduc-bench
    benchmarks/bench_cfr.cpp
    benchmarks/bench_game.cpp
    ${SOURCES}
)
```

## 8. Performance Optimizations

### 8.1 Compiler Optimizations

- `-O3`: Maximum optimization level
- `-march=native`: Use CPU-specific instructions (AVX, SSE)
- `-flto`: Link-time optimization
- `-ffast-math`: Aggressive floating-point optimizations (use with caution)

### 8.2 Cache Optimization

- Align CFRNode to 64-byte cache lines
- Store nodes in contiguous array (spatial locality)
- Minimize pointer chasing (use indices instead of pointers)
- Pack frequently-accessed data together

### 8.3 Branch Prediction

- Use branchless code for terminal evaluation
- Mark unlikely branches with `__builtin_expect` (GCC/Clang)
- Minimize conditional branches in hot loop

### 8.4 Memory Access Patterns

- Sequential access to node array (cache-friendly)
- Avoid random access patterns
- Prefetch next node if predictable

## 9. Testing Strategy

### 9.1 Unit Tests

- **Game State Tests**: Legal actions, state transitions, terminal evaluation
- **InfoSet Tests**: Encoding/decoding, uniqueness, collision detection
- **CFR Tests**: Regret updates, strategy computation, convergence
- **Exploitability Tests**: Best-response calculation, Nash distance

### 9.2 Integration Tests

- **Convergence Test**: Run 100k iterations, verify exploitability < 1 mbb/hand
- **Equilibrium Test**: Validate Jack/Queen/King strategies match theory
- **Symmetry Test**: Verify player 0 and player 1 strategies are symmetric

### 9.3 Performance Tests

- **Iteration Speed**: Measure iterations per second
- **Memory Usage**: Verify < 100 KB total allocation
- **Cache Efficiency**: Profile cache misses with perf/valgrind

## 10. Correctness Properties

### Property 1: Regret Matching Produces Valid Probability Distribution
**Validates: Requirements FR-1.2**

For any information set, the strategy computed by regret matching must:
- Sum to 1.0 (±epsilon for floating-point error)
- All probabilities in [0, 1]

### Property 2: Terminal Utilities Sum to Zero
**Validates: Requirements FR-1.1**

For any terminal state, the sum of utilities for both players must equal zero (zero-sum game).

### Property 3: Information Set Encoding is Injective
**Validates: Requirements FR-1.1**

Different game states that are distinguishable to a player must produce different information set keys.

### Property 4: Exploitability Decreases Monotonically (in expectation)
**Validates: Requirements FR-3.2**

As CFR iterations increase, exploitability should decrease (or remain constant) in expectation.

### Property 5: Best-Response Value ≥ Nash Equilibrium Value
**Validates: Requirements FR-3.1**

The best-response value against any strategy must be at least as good as the Nash equilibrium value.

### Property 6: Legal Actions Mask is Non-Empty
**Validates: Requirements FR-1.2**

At any non-terminal state, at least one action must be legal.

### Property 7: Jack Folds to Aggression in Equilibrium
**Validates: Requirements FR-3.3**

In a converged strategy, Jack (weakest hand) should fold with high probability (>80%) when facing a bet pre-flop.

### Property 8: Strategy Sum Accumulation is Non-Negative
**Validates: Requirements FR-1.2**

Strategy sums must always be non-negative (probabilities weighted by reach).

## 11. Open Questions

1. **Serialization Format**: Binary or text-based for strategy export?
   - Recommendation: Binary for performance, optional text export for debugging

2. **CFR+ vs Vanilla CFR**: Enable CFR+ by default?
   - Recommendation: Start with Vanilla CFR, add CFR+ as compile-time option

3. **Floating-Point Precision**: Use float (32-bit) or double (64-bit)?
   - Recommendation: float for storage, double for accumulation

4. **Multi-Threading**: Add OpenMP pragmas for parallel tree traversal?
   - Recommendation: Out of scope for initial implementation, add later

5. **SIMD**: Vectorize regret updates across multiple nodes?
   - Recommendation: Out of scope for initial implementation, add later
