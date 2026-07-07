/**
 * @file tests/test_simd.cpp
 * @brief Intense validation testing our AVX2 Structure-of-Arrays (SoA) layout.
 * @details Verifies that processing 4 games simultaneously within StateBatch4 matches 
 *          the exact deterministic rules of single isolated bitboard physics.
 */

#include <gtest/gtest.h>
#include "fog_state.h"
#include "../src/kernel/move_gen.cpp"

TEST(FogSimdCoreTest, TestBatchedOccupancyRecalculation) {
    alignas(64) fog::StateBatch4 batch;
    
    // Wipe all bits clean
    std::memset(&batch, 0, sizeof(fog::StateBatch4));

    // Inject individual test piece parameters into separate lanes[cite: 4]
    // Game Lane 0: White Pawn on e2 (Square 12)[cite: 3]
    batch.pieces[0][0][0] = (1ULL << 12);
    // Game Lane 1: White Pawn on e4 (Square 28)[cite: 3]
    batch.pieces[0][0][1] = (1ULL << 28);
    // Game Lane 2: Black Knight on b8 (Square 57)[cite: 3]
    batch.pieces[1][1][2] = (1ULL << 57);
    // Game Lane 3: White King on e1 (Square 4)[cite: 3, 4]
    batch.pieces[0][5][3] = (1ULL << 4);

    // Trigger the AVX2 SIMD compilation loop execution
    fog::kernel::update_batched_occupancy(&batch);

    // Verify each lane resolved its occupancy completely separate from its neighbors
    EXPECT_EQ(batch.white_occ[0], (1ULL << 12));
    EXPECT_EQ(batch.white_occ[1], (1ULL << 28));
    EXPECT_EQ(batch.black_occ[2], (1ULL << 57));
    EXPECT_EQ(batch.white_occ[3], (1ULL << 4));

    // Verify Total Occupancy layer aggregation across all 4 worlds[cite: 4]
    EXPECT_EQ(batch.total_occ[0], batch.white_occ[0]);
    EXPECT_EQ(batch.total_occ[2], batch.black_occ[2]);
}