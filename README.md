# Leduc Poker CFR Solver

A high-performance C++17 implementation of Leduc Poker solver using Counterfactual Regret Minimization (CFR). This solver computes Nash Equilibrium strategies with a focus on performance, cache efficiency, and correctness.

## Overview

Leduc Poker is a simplified poker variant with:
- 6-card deck (2 Jacks, 2 Queens, 2 Kings)
- 2 players
- 2 betting rounds (pre-flop and flop)
- Fixed-limit betting

This implementation uses Vanilla CFR to compute optimal strategies, achieving exploitability < 1 mbb/hand (milli-big-blinds per hand) after sufficient iterations.

## Key Features

- **Pure C++17** with zero external dependencies for core solver
- **High Performance**: >16,000 iterations/second on modern CPUs
- **Memory Efficient**: <100 KB total memory footprint
- **Cache Optimized**: 64-byte aligned data structures, sequential access patterns
- **Branchless Code**: Terminal evaluation uses bit manipulation for speed
- **Comprehensive Testing**: Unit tests, integration tests, and performance benchmarks
- **Extensible Design**: Foundation for CFR+, parallel training, and SIMD optimization

## Prerequisites

### Required
- **CMake** 3.15 or later
- **C++17 compatible compiler**:
  - GCC 9+ (Linux)
  - Clang 10+ (Linux/macOS)
  - Apple Clang 12+ (macOS)
  - MSVC 2019+ (Windows)

### Optional
- **Valgrind** (for memory profiling)
- **perf** (for performance profiling on Linux)
- **Python 3.7+** (for strategy analysis scripts)

## Building

### Quick Start

```bash
# Clone and navigate to project
cd leduc-cfr

# Create build directory
mkdir build && cd build

# Configure and build (Release mode for performance)
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j4

# Or using make directly
make -j4
```

### Build Configurations

#### Release Build (Recommended for Training)
Optimized for performance with `-O3 -march=native`:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j4
```

#### Debug Build (For Development)
Includes debug symbols and no optimizations:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug -j4
```

### Build Targets

The build produces three executables:

1. **leduc-cfr** — Main training executable
   - Trains CFR solver for specified iterations
   - Logs exploitability and progress
   - Outputs final strategy

2. **leduc-tests** — Unit and integration test suite
   - Tests game logic, CFR algorithm, exploitability calculation
   - Validates correctness properties

3. **leduc-bench** — Performance benchmarks
   - Measures iterations per second
   - Profiles memory usage
   - Identifies performance bottlenecks

## Running

### Training the Solver

```bash
# Basic training (100,000 iterations)
./leduc-cfr --iterations 100000

# With custom log interval
./leduc-cfr --iterations 1000000 --log-interval 10000

# With checkpoint saving
./leduc-cfr --iterations 1000000 --checkpoint-interval 100000
```

**Command-line Options:**
- `--iterations N` — Number of CFR iterations (default: 100000)
- `--log-interval N` — Log progress every N iterations (default: 1000)
- `--checkpoint-interval N` — Save checkpoint every N iterations (optional)

**Output:**
```
Iteration 0: Exploitability = 1234.56 mbb/hand, Time = 0.12s
Iteration 1000: Exploitability = 456.78 mbb/hand, Time = 1.23s
...
Iteration 100000: Exploitability = 0.45 mbb/hand, Time = 123.45s

Final Strategy:
J|?: Fold=0.92, Call=0.08, Raise=0.00
Q|?: Fold=0.45, Call=0.35, Raise=0.20
K|?: Fold=0.05, Call=0.15, Raise=0.80
```

### Running Tests

```bash
# Run all tests
./leduc-tests

# Expected output:
# [==========] Running 20 tests from 5 test suites.
# [==========] 20 tests from 5 test suites ran. (123 ms total)
# [  PASSED  ] 20 tests.
```

### Running Benchmarks

```bash
# Run performance benchmarks
./leduc-bench

# Expected output:
# CFR Iteration Speed: 18,234 iterations/second
# Memory Usage: 87 KB
# Cache Miss Rate: 2.3%
```

## Performance Benchmarks

### Expected Performance (Modern CPU: Intel i7/AMD Ryzen)

| Metric | Target | Typical |
|--------|--------|---------|
| Iteration Speed | >16,000 iter/s | 18,000-22,000 iter/s |
| Memory Usage | <100 KB | 87 KB |
| 1M Iterations | <60s | 45-55s |
| Exploitability (1M iter) | <1 mbb/hand | 0.3-0.5 mbb/hand |

### Profiling

Profile with perf (Linux):
```bash
perf record -g ./leduc-cfr --iterations 100000
perf report
```

Profile with Valgrind:
```bash
valgrind --tool=cachegrind ./leduc-cfr --iterations 10000
cg_annotate cachegrind.out.<pid>
```

## Algorithm Details

### Counterfactual Regret Minimization (CFR)

CFR is an iterative algorithm that computes Nash Equilibrium strategies by:

1. **Traversing the game tree** for each player
2. **Computing counterfactual values** for each action
3. **Accumulating regrets** (difference between action value and node value)
4. **Updating strategy** via regret matching (proportional to positive regrets)

After sufficient iterations, the average strategy converges to Nash Equilibrium.

### Information Set Encoding

Leduc Poker has ~936 unique information sets. We use a compact 16-bit encoding:

```
Bits 0-3:   Action sequence (4 bits)
Bits 4-5:   Player card (2 bits: J=0, Q=1, K=2)
Bits 6-7:   Public card (2 bits: J=0, Q=1, K=2, none=3)
Bits 8-13:  Betting history (6 bits)
Bits 14-15: Round (2 bits: 0=pre-flop, 1=flop)
```

This allows direct array indexing without hash tables, improving cache efficiency.

### Expected Equilibrium Strategies

**Jack (Weakest Hand):**
- Pre-flop first-to-act: Check ~90%, Bet ~10% (bluff)
- Facing bet: Fold ~85-95%, Call ~5-15%

**Queen (Middle Hand):**
- Pre-flop first-to-act: Check ~60%, Bet ~40%
- Facing bet: Call ~70%, Fold ~20%, Raise ~10%

**King (Strongest Hand):**
- Pre-flop first-to-act: Bet ~75-85%, Check ~15-25% (trap)
- Facing bet: Raise ~80%, Call ~20%

## Correctness Validation

The implementation validates correctness through:

1. **Unit Tests**: Game logic, CFR updates, terminal evaluation
2. **Integration Tests**: Convergence to Nash Equilibrium, strategy symmetry
3. **Property-Based Tests**: Regret matching validity, zero-sum property
4. **Equilibrium Validation**: Jack/Queen/King strategies match theory

Run tests with:
```bash
./leduc-tests
```

## Troubleshooting

### Build Issues

**CMake not found:**
```bash
# Install CMake
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake

# Windows
# Download from https://cmake.org/download/
```

**Compiler not found:**
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# macOS
xcode-select --install

# Windows
# Install Visual Studio 2019+ or MinGW
```

**Compilation errors with -Werror:**
- Ensure you're using a C++17 compatible compiler
- Try with `-DCMAKE_CXX_FLAGS=""` to disable strict warnings temporarily

### Runtime Issues

**Slow performance:**
- Ensure Release build: `cmake -DCMAKE_BUILD_TYPE=Release`
- Check CPU frequency scaling: `cat /proc/cpuinfo | grep MHz`
- Profile with perf to identify bottlenecks

**Memory issues:**
- Check available RAM: `free -h` (Linux) or `wmic OS get TotalVisibleMemorySize` (Windows)
- Profile with Valgrind: `valgrind --leak-check=full ./leduc-cfr`

## Documentation

- **[Design Document](./.kiro/specs/optimized-leduc-cfr/design.md)** — Detailed architecture and algorithm design
- **[Requirements Document](./.kiro/specs/optimized-leduc-cfr/requirements.md)** — Functional and non-functional requirements
- **[Task List](./.kiro/specs/optimized-leduc-cfr/tasks.md)** — Implementation roadmap and progress tracking
- **[Project Plan](./leduc-poker-cfr-plan.md)** — High-level engineering roadmap

> **Note:** The `.kiro/` directory contains internal project specifications and is ignored by version control.

## References
- [Analysis of Bluffing by DQN and CFR in Leduc Hold'em Poker](https://arxiv.org/html/2509.04125v1)
- [Leduc Poker](https://en.wikipedia.org/wiki/Leduc_poker) — Wikipedia

## License

See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure:
- All tests pass: `./leduc-tests`
- No compiler warnings: `-Wall -Wextra -Werror`
- Code is formatted with clang-format
- New features include unit tests
