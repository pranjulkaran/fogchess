#include <iostream>
#include <chrono>
#include <vector>
#include "fog_sdk.h"

int main() {
    // 2,500 batches * 4 lanes = 10,000 parallel games
    const uint32_t NUM_BATCHES = 64; 
    const uint32_t TOTAL_GAMES = NUM_BATCHES * 4;
    const uint32_t NUM_STEPS = 40000;

    std::cout << "[Benchmark] Booting engine with " << TOTAL_GAMES << " parallel environments..." << std::endl;
    FogEngineHandle engine = fog_engine_init(NUM_BATCHES);

    std::vector<uint32_t> batch_indices(NUM_BATCHES);
    for(uint32_t i = 0; i < NUM_BATCHES; ++i) {
        batch_indices[i] = i;
    }

    fog_batch_reset(engine, batch_indices.data(), NUM_BATCHES);

    // Using the raw packed command for e2-e4 (5121) you generated earlier
    std::vector<uint32_t> commands(TOTAL_GAMES, 5121); 

    std::cout << "[Benchmark] Executing " << NUM_STEPS << " batched steps..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for(uint32_t s = 0; s < NUM_STEPS; ++s) {
        // Steps 10,000 games simultaneously across the worker threads[cite: 39]
        fog_batch_step(engine, batch_indices.data(), commands.data(), TOTAL_GAMES);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    double total_steps = static_cast<double>(TOTAL_GAMES) * NUM_STEPS;
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Total Environment Steps: " << total_steps << std::endl;
    std::cout << "Time Elapsed:            " << elapsed.count() << " seconds" << std::endl;
    std::cout << "SIMD Kernel Throughput:  " << (total_steps / elapsed.count()) << " SPS (Steps Per Second)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    fog_engine_shutdown(engine);
    return 0;
}