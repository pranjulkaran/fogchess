/**
 * @file tests/test_kernel_core.cpp
 * @brief Aggressive integration tests validating batched move generation, initialization, and threats.
 */

#include <gtest/gtest.h>
#include <cstring>
#include "fog_state.h"
#include "fog_command.h"

// Forward declarations of the core kernel mechanics we want to test
namespace fog {
namespace kernel {
    void init_engine_tables();
    void batched_reset_board(StateBatch4* batch);
    uint32_t generate_pseudo_moves_lane(const StateBatch4* batch, int lane, uint32_t* out_commands, uint32_t max_moves);
    bool is_square_attacked_lane(const StateBatch4* batch, int lane, int sq, int attacker_color);
    uint32_t batched_is_king_captured(const StateBatch4* batch, int target_color);
}
}

class FogKernelCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure static tables are initialized before running any mathematical test lane
        fog::kernel::init_engine_tables();
    }
};

/**
 * @brief Assures that resetting the board populates exactly the standard starting positions 
 *        and provides the classic 20 opening moves for white across all lanes.
 */
TEST_F(FogKernelCoreTest, TestStandardInitializationAndOpeningMoves) {
    alignas(64) fog::StateBatch4 batch;
    fog::kernel::batched_reset_board(&batch);

    // Verify all 4 parallel games independent turn trackers are zero (White's Turn)
    for (int lane = 0; lane < 4; ++lane) {
        EXPECT_EQ(batch.active_turn[lane], 0);
        
        // Assert that the full initial pawn line matches standard 0x000000000000FF00ULL
        EXPECT_EQ(batch.pieces[0][0][lane], 0x000000000000FF00ULL);
        EXPECT_EQ(batch.pieces[1][0][lane], 0x00FF000000000000ULL);

        // Verify that move generation extracts exactly the 20 legal opening moves
        uint32_t commands[256];
        uint32_t count = fog::kernel::generate_pseudo_moves_lane(&batch, lane, commands, 256);
        EXPECT_EQ(count, 20); 
    }
}

/**
 * @brief Verifies that threat detection cleanly checks attacks without bleeding across lanes.
 */
TEST_F(FogKernelCoreTest, TestThreatDetectionIsolation) {
    alignas(64) fog::StateBatch4 batch;
    std::memset(&batch, 0, sizeof(fog::StateBatch4));

    // Lane 0 setup: White King on e1 (4), Enemy Black Rook on e8 (60)
    batch.pieces[0][5][0] = (1ULL << 4);
    batch.pieces[1][3][0] = (1ULL << 60);
    batch.total_occ[0] = batch.pieces[0][5][0] | batch.pieces[1][3][0];

    // Lane 1 setup: White King on e1 (4), but block the ray with a White Pawn on e2 (12)
    batch.pieces[0][5][1] = (1ULL << 4);
    batch.pieces[0][0][1] = (1ULL << 12);
    batch.pieces[1][3][1] = (1ULL << 60);
    batch.total_occ[1] = batch.pieces[0][5][1] | batch.pieces[0][0][1] | batch.pieces[1][3][1];

    // Assert Lane 0: Square e1 (4) is aggressively attacked by Black (1)
    EXPECT_TRUE(fog::kernel::is_square_attacked_lane(&batch, 0, 4, 1));

    // Assert Lane 1: Square e1 (4) is completely SAFE because the white pawn clips the rook's vision ray
    EXPECT_FALSE(fog::kernel::is_square_attacked_lane(&batch, 1, 4, 1));
}

/**
 * @brief Assures that the king capture tracking mask behaves correctly when a board collapses.
 */
TEST_F(FogKernelCoreTest, TestTerminalStateKingCaptureMask) {
    alignas(64) fog::StateBatch4 batch;
    std::memset(&batch, 0, sizeof(fog::StateBatch4));

    // Populate Kings across lanes 0, 1, and 2 for White
    batch.pieces[0][5][0] = (1ULL << 4);
    batch.pieces[0][5][1] = (1ULL << 4);
    batch.pieces[0][5][2] = (1ULL << 4);
    // Lane 3 is left empty (White King is missing/captured)

    // Evaluate terminal status mask for White (Color 0)
    uint32_t dead_mask = fog::kernel::batched_is_king_captured(&batch, 0);

    // Expecting only bit 3 to be flipped high (value = 1 << 3 = 8)
    EXPECT_EQ(dead_mask, 8);
}