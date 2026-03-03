#pragma once

#include <bits/stdc++.h>
#include "types.h"

namespace leduc
{

    // ---------------------------------------------------------------------------
    // CFRNode – 64-byte cache-line-aligned information set node
    // ---------------------------------------------------------------------------
    // One CFRNode stores all CFR data for a single information set:
    //   - Cumulative regrets      (regret_sum)
    //   - Cumulative strategy     (strategy_sum)   – for average-strategy extraction
    //   - Current iteration strat (strategy)        – recomputed from regrets each iter
    //   - Metadata: num_actions, action_mask, infoset_key
    //
    // Layout (64 bytes == 1 cache line):
    //   float  regret_sum[3]    12 bytes  (offset  0)
    //   float  strategy_sum[3]  12 bytes  (offset 12)
    //   float  strategy[3]      12 bytes  (offset 24)
    //   uint8  num_actions       1 byte   (offset 36)
    //   uint8  action_mask       1 byte   (offset 37)
    //   uint16 infoset_key       2 bytes  (offset 38)
    //   uint8  padding[24]      24 bytes  (offset 40)
    //                           --------
    //                           64 bytes  total

    struct alignas(64) CFRNode
    {
        // ------------------------------------------------------------------
        // Per-action accumulators
        // ------------------------------------------------------------------

        /// Cumulative counterfactual regret for each action index.
        /// Index: 0 = FOLD, 1 = CALL, 2 = RAISE  (matches Action enum values).
        float regret_sum[NUM_ACTIONS]; // 12 bytes

        /// Cumulative weighted strategy for each action.
        /// Used to compute the time-averaged (equilibrium) strategy.
        float strategy_sum[NUM_ACTIONS]; // 12 bytes

        /// Current-iteration strategy (probability distribution over actions).
        /// Derived via regret matching; recomputed at the start of each iteration.
        float strategy[NUM_ACTIONS]; // 12 bytes

        // ------------------------------------------------------------------
        // Metadata
        // ------------------------------------------------------------------

        /// Number of legal actions at this information set (1–3).
        uint8_t num_actions; // 1 byte

        /// Bitmask of legal actions: bit 0 = FOLD, bit 1 = CALL, bit 2 = RAISE.
        uint8_t action_mask; // 1 byte

        /// The 16-bit infoset key that maps to this node (for debugging).
        uint16_t infoset_key; // 2 bytes

        // ------------------------------------------------------------------
        // Explicit padding to reach 64 bytes
        // ------------------------------------------------------------------
        uint8_t padding[24]; // 24 bytes

        // ------------------------------------------------------------------
        // Helper methods
        // ------------------------------------------------------------------

        /// Zero-initialise all accumulators and metadata.
        void reset() noexcept
        {
            for (int i = 0; i < NUM_ACTIONS; ++i)
            {
                regret_sum[i] = 0.0f;
                strategy_sum[i] = 0.0f;
                strategy[i] = 0.0f;
            }
            num_actions = 0;
            action_mask = 0;
            infoset_key = 0;
            for (int i = 0; i < 24; ++i)
                padding[i] = 0;
        }

        /// Recompute strategy from regret_sum using regret matching.
        /// Actions with non-positive regret get zero probability; the remainder
        /// is normalised.  Falls back to uniform over legal actions if all
        /// regrets are ≤ 0.
        void update_strategy() noexcept
        {
            float pos_sum = 0.0f;
            for (int i = 0; i < NUM_ACTIONS; ++i)
            {
                float r = (action_mask >> i) & 1 ? regret_sum[i] : 0.0f;
                strategy[i] = (r > 0.0f) ? r : 0.0f;
                pos_sum += strategy[i];
            }
            if (pos_sum > 0.0f)
            {
                for (int i = 0; i < NUM_ACTIONS; ++i)
                    strategy[i] /= pos_sum;
            }
            else
            {
                // Uniform over legal actions
                float uniform = 1.0f / static_cast<float>(num_actions);
                for (int i = 0; i < NUM_ACTIONS; ++i)
                    strategy[i] = ((action_mask >> i) & 1) ? uniform : 0.0f;
            }
        }

        /// Return the time-averaged (Nash) strategy normalised from strategy_sum.
        /// Writes result into out[NUM_ACTIONS].  Falls back to uniform if sum is 0.
        void get_average_strategy(float out[NUM_ACTIONS]) const noexcept
        {
            float total = 0.0f;
            for (int i = 0; i < NUM_ACTIONS; ++i)
                total += strategy_sum[i];
            if (total > 0.0f)
            {
                for (int i = 0; i < NUM_ACTIONS; ++i)
                    out[i] = strategy_sum[i] / total;
            }
            else
            {
                float uniform = 1.0f / static_cast<float>(num_actions);
                for (int i = 0; i < NUM_ACTIONS; ++i)
                    out[i] = ((action_mask >> i) & 1) ? uniform : 0.0f;
            }
        }
    };

    // Compile-time layout checks.
    static_assert(sizeof(CFRNode) == 64, "CFRNode must be exactly 64 bytes");
    static_assert(alignof(CFRNode) == 64, "CFRNode must be 64-byte aligned");
    static_assert(offsetof(CFRNode, regret_sum) == 0, "regret_sum at offset 0");
    static_assert(offsetof(CFRNode, strategy_sum) == 12, "strategy_sum at offset 12");
    static_assert(offsetof(CFRNode, strategy) == 24, "strategy at offset 24");
    static_assert(offsetof(CFRNode, num_actions) == 36, "num_actions at offset 36");
    static_assert(offsetof(CFRNode, action_mask) == 37, "action_mask at offset 37");
    static_assert(offsetof(CFRNode, infoset_key) == 38, "infoset_key at offset 38");

} // namespace leduc
