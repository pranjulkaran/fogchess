/**
 * @file src/main.cpp
 * @brief Bootstrapper to test the FogEngine C-ABI and batch execution.
 */

#include <iostream>
#include "fog_sdk.h"

int main() {
    std::cout << "[FogEngine] Booting deterministic runtime..." << std::endl;
    FogEngineHandle engine = fog_engine_init(1024);
    
    // 1. Reset batch index 0 (which contains 4 parallel game lanes) to standard start position
    uint32_t batch_indices[] = {0};
    fog_batch_reset(engine, batch_indices, 1);
    std::cout << "[FogEngine] Batch 0 reset to standard chess positions." << std::endl;

    // 2. Generate moves for Lane 0 of that batch
    uint32_t out_commands[256];
    uint32_t out_count = 0;
    fog_get_moves(engine, 0, 0, out_commands, &out_count);

    std::cout << "[SUCCESS] Generated " << out_count << " legal moves for the starting position!" << std::endl;

    fog_engine_shutdown(engine);
    return 0;
}