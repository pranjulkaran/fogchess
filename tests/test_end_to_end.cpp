#include <gtest/gtest.h>
#include "fog_sdk.h"
#include <vector>

class FogEngineTest : public ::testing::Test {
protected:
    FogEngineHandle engine;

    void SetUp() override {
        engine = fog_engine_init(1024); // Allocate 1024 batches
        ASSERT_NE(engine, nullptr) << "Engine failed to initialize.";
    }

    void TearDown() override {
        if (engine) {
            fog_engine_shutdown(engine);
        }
    }
};

TEST_F(FogEngineTest, BatchResetAndMoveGeneration) {
    uint32_t batch_indices[] = {0, 1};
    int32_t res = fog_batch_reset(engine, batch_indices, 2);
    EXPECT_EQ(res, 0) << "Batch reset failed.";

    uint32_t moves[256];
    uint32_t count = 0;
    
    // Test opening moves for White in Lane 0 of Batch 0
    res = fog_get_moves(engine, 0, 0, moves, &count);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(count, 20) << "Standard chess should have exactly 20 opening moves for White.";
}

TEST_F(FogEngineTest, ZeroAllocationTensorExtraction) {
    uint32_t batch_indices[] = {0};
    fog_batch_reset(engine, batch_indices, 1);

    // Pre-allocate the exact flat buffers expected by Python
    std::vector<float> layer3_tensor(14 * 8 * 8, 0.0f);
    std::vector<int8_t> true_state(64, 0);

    // Empty visibility and scout arrays for initial test
    uint64_t full_visibility = ~0ULL;
    std::vector<int32_t> empty_scouted;

    int32_t res_tensor = fog_get_layer3_tensor(
        engine, 0, 0, 
        full_visibility, 
        empty_scouted.data(), 0, 
        empty_scouted.data(), 0, 
        0, layer3_tensor.data()
    );
    EXPECT_EQ(res_tensor, 0);

    int32_t res_true = fog_get_true_state_matrix(engine, 0, 0, true_state.data());
    EXPECT_EQ(res_true, 0);

    // Verify a White Pawn (token 1) exists at tensor index 48 (Flipped a2)
    EXPECT_EQ(true_state[48], 1);
    
    // Verify a Black Pawn (token -1) exists at tensor index 8 (Flipped a7)
    EXPECT_EQ(true_state[8], -1);
}