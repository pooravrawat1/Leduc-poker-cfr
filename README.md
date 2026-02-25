# CFR for Leduc Poker

Game-theoretic poker agent using Deep Counterfactual Regret Minimization (CFR) to solve Leduc Poker via self-play.

## Overview

This project implements CFR with external sampling, evaluating convergence via exploitability against best-response opponents.

## Architecture (tentative)

                ┌──────────────────────┐
                │      User / Tester   │
                │  CLI or Web Interface│
                └──────────┬───────────┘
                           │
                ┌──────────▼───────────┐
                │     Agent Interface  │
                │  (Average Strategy)  │
                └──────────┬───────────┘
                           │
                ┌──────────▼───────────┐
                │   Strategy Module    │
                │ extract avg strategy │
                └──────────┬───────────┘
                           │
        ┌──────────────────▼──────────────────┐
        │            CFR Engine               │
        │ recursion • regret matching • tree  │
        └──────────────────┬──────────────────┘
                           │
        ┌──────────────────▼──────────────────┐
        │        Information Set Layer        │
        │ abstraction + state encoding        │
        └──────────────────┬──────────────────┘
                           │
                ┌──────────▼───────────┐
                │   RLCard Environment │
                │   Leduc Poker Engine │
                └──────────────────────┘
