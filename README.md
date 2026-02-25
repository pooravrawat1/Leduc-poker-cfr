# Leduc Poker CFR Solver (C++17)

A C++ implementation of Leduc Poker solver using Counterfactual Regret Minimization (CFR).

## Key Features

- **Pure C++17** with minimal dependencies (only CMake and a C++ compiler)
- **Zero external game libraries** — entirely from scratch
- **Vanilla CFR** solver with full support for imperfect information
- **Tabular representation** — efficient for Leduc Poker's small game tree
- **Extensible design** — stubs prepared for Deep CFR, parallel training, etc.

## Build & Run

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2019+)
- Optional: `nlohmann/json` (header-only, auto-included)

### Build

```bash
mkdir build && cd build
cmake ..
make -j4
```

### Run Training

```bash
./cfr_train --iterations 100000 --output ../checkpoints/
```

### Play Against Solver

```bash
./cfr_play --checkpoint ../checkpoints/checkpoint_100000.json --games 10 --human-player 0
```

### Run Tests

```bash
./cfr_tests
```

## Project Plan

See [leduc-poker-cfr-plan.md](leduc-poker-cfr-plan.md) for full engineering roadmap.
