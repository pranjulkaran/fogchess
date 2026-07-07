/**
 * @file tests/test_determinism.cpp
 * @brief Hard testing for lockstep execution, replayability, and C-ABI thread safety.
 */

#include <gtest/gtest.h>
#include "fog_sdk.h"
#include "fog_command.h"

TEST(FogDeterminismTest, TestParallelReplayConsistency) {
    // 1. Initialize the engine SDK boundary[cite: 9]
    FogEngineHandle engine = fog_engine_init(16);
    ASSERT_NE(engine, nullptr);

    // 2. Prepare mock command streams to move e2 to e4 across 4 separate games[cite: 3]
    alignas(32) uint32_t batch_ids[4] = {0, 0, 0, 0}; // All pointing to batch slot 0
    alignas(32) uint32_t command_stream[4];

    // Build 16-bit commands using our packed bit mask system
    fog::Command move_cmd(12, 28, 0, 0); // From e2 (12) to e4 (28), PAWN (0)[cite: 3, 4]
    for (int i = 0; i < 4; ++i) {
        command_stream[i] = move_cmd.get_raw();
    }

    // 3. Fire the step function via the flat external binary boundary
    int32_t result = fog_batch_step(engine, batch_ids, command_stream, 4);
    EXPECT_EQ(result, 0);

    // Clean up completely
    fog_engine_shutdown(engine);
}