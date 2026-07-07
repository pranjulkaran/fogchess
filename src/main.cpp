/**
 * @file src/main.cpp
 * @brief Bootstrapper to test the FogEngine C-ABI and batch execution.
 */

#include <iostream>
#include <vector>
#include "fog_sdk.h"
#include "fog_command.h"

int main() {
    std::cout << "[FogEngine] Booting deterministic runtime..." << std::endl;

    // 1. Initialize the engine with an arena capacity of 1,024 batches (4,096 games)
    FogEngineHandle engine = fog_engine_init(1024);
    if (!engine) {
        std::cerr << "[ERROR] Failed to initialize FogEngine." << std::endl;
        return 1;
    }
    std::cout << "[FogEngine] Arena allocated successfully." << std::endl;

    // 2. Prepare a batch of 8 parallel environments to step
    uint32_t count = 8;
    std::vector<uint32_t> batch_indices = {0, 0, 0, 0, 1, 1, 1, 1}; // Games targeting batch slot 0 and 1
    std::vector<uint32_t> commands;

    // Create a mock command: Move a Pawn from e2 (Square 12) to e4 (Square 28)[cite: 3, 4]
    fog::Command mock_move(12, 28, 0, 0); 
    
    for (uint32_t i = 0; i < count; ++i) {
        commands.push_back(mock_move.get_raw());
    }

    // 3. Execute the batched step through the SDK boundary
    std::cout << "[FogEngine] Executing batch step for " << count << " environments..." << std::endl;
    int32_t result = fog_batch_step(engine, batch_indices.data(), commands.data(), count);

    if (result == 0) {
        std::cout << "[SUCCESS] Batch stepped perfectly via SIMD thread pool." << std::endl;
    } else {
        std::cerr << "[ERROR] Batch step failed with code: " << result << std::endl;
    }

    // 4. Clean up
    fog_engine_shutdown(engine);
    std::cout << "[FogEngine] Runtime shut down cleanly." << std::endl;

    return 0;
}