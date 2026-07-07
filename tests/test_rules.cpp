/**
 * @file tests/test_rules.cpp
 * @brief Validates complex game rules and state hashing.
 */

#include <gtest/gtest.h>
#include <cstring>
#include "fog_state.h"
#include "fog_command.h"
#include "../src/kernel/hasher.hpp"

// Forward declare the function from rules.cpp
namespace fog { namespace kernel {
    void apply_batched_commands(StateBatch4* batch, const uint32_t* commands);
}}

TEST(FogRulesTest, TestCastlingAndHashing) {
    alignas(64) fog::StateBatch4 batch;
    std::memset(&batch, 0, sizeof(fog::StateBatch4));

    // Setup Lane 0: White King on e1 (4), White Rook on h1 (7)[cite: 3]
    batch.pieces[0][5][0] = (1ULL << 4);
    batch.pieces[0][3][0] = (1ULL << 7);
    batch.active_turn[0] = 0; // White's turn

    // Record the initial hash
    uint64_t initial_hash = fog::kernel::hash_lane(&batch, 0);

    // Command: White King-side Castle[cite: 4]
    fog::Command castle_cmd(4, 6, 5, fog::MOVE_KING_CASTLE);
    uint32_t commands[4] = { castle_cmd.get_raw(), 0, 0, 0 };

    // Apply the rule
    fog::kernel::apply_batched_commands(&batch, commands);

    // Verify physics: King is now on g1 (6), Rook is on f1 (5)[cite: 3]
    EXPECT_EQ(batch.pieces[0][5][0], (1ULL << 6));
    EXPECT_EQ(batch.pieces[0][3][0], (1ULL << 5));
    
    // Verify turn flipped
    EXPECT_EQ(batch.active_turn[0], 1); 

    // Verify hash changed deterministically
    uint64_t post_hash = fog::kernel::hash_lane(&batch, 0);
    EXPECT_NE(initial_hash, post_hash);
}