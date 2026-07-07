/**
 * @file src/sdk/bridge.cpp
 * @brief Authoritative C-ABI bridge translating language-agnostic primitives to internal engine operations.
 * @details Implements fog_sdk.h entirely, managing safe type casting and ensuring no C++ exceptions 
 *          propagate across the application binary interface boundary.
 */

#include "fog_sdk.h"
#include "../runtime/scheduler.cpp" // Core scheduler implementation compiled directly into this unit

// --- KERNEL EXTERN FUNCTIONS LINKING FROM THE RULES/TABLE MODULES ---
namespace fog {
namespace kernel {
    void init_engine_tables();
    void batched_reset_board(StateBatch4* batch);
    uint32_t generate_pseudo_moves_lane(const StateBatch4* batch, int lane, uint32_t* out_commands, uint32_t max_moves);
    uint32_t batched_is_king_captured(const StateBatch4* batch, int target_color);
}
}

extern "C" {

/**
 * @brief Initializes the global engine context, triggers static lookup calculations, and allocates memory.
 */
FogEngineHandle fog_engine_init(uint32_t max_batches) {
    try {
        // Compute static leap steps and sliding ray bitmasks exactly once at boot
        fog::kernel::init_engine_tables();
        
        // Instantiate the runtime memory arena and task processor on the heap
        auto* scheduler = new fog::runtime::TaskScheduler(max_batches);
        return reinterpret_cast<FogEngineHandle>(scheduler);
    } catch (...) {
        return nullptr;
    }
}

/**
 * @brief Gracefully purges the global memory arena and kills the task processor.
 */
void fog_engine_shutdown(FogEngineHandle engine) {
    if (engine) {
        auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
        delete scheduler;
    }
}

/**
 * @brief Spits commands directly out across the worker threads in groups of 4 for SIMD sweeps.
 */
int32_t fog_batch_step(FogEngineHandle engine, const uint32_t* batch_indices, const uint32_t* commands, uint32_t count) {
    if (!engine || !batch_indices || !commands || count == 0) return -1;
    if (count % 4 != 0) return -2; // Structural enforcement: must match SIMD hardware lanes

    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    scheduler->step_all_parallel(batch_indices, commands, count);
    
    return 0;
}

/**
 * @brief Clears bitboard planes back to standard initial chess setups without hitting the OS heap.
 */
int32_t fog_batch_reset(FogEngineHandle engine, const uint32_t* batch_indices, uint32_t count) {
    if (!engine || !batch_indices || count == 0) return -1;
    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    
    for (uint32_t i = 0; i < count; ++i) {
        fog::StateBatch4* batch = scheduler->get_arena().get_batch(batch_indices[i]);
        fog::kernel::batched_reset_board(batch);
    }
    return 0;
}

/**
 * @brief Generates pseudo-legal command arrays out of an isolated lane inside a batch slot.
 */
int32_t fog_get_moves(FogEngineHandle engine, uint32_t batch_index, uint32_t lane, uint32_t* out_commands, uint32_t* out_count) {
    if (!engine || !out_commands || !out_count || lane > 3) return -1;
    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    
    const fog::StateBatch4* batch = scheduler->get_arena().get_batch(batch_index);
    *out_count = fog::kernel::generate_pseudo_moves_lane(batch, lane, out_commands, 256); // MAX_MOVES upper bound
    
    return 0;
}

/**
 * @brief Runs low-overhead termination check maps over a batched frame slot.
 */
int32_t fog_check_terminations(FogEngineHandle engine, uint32_t batch_index, uint32_t* out_mask) {
    if (!engine || !out_mask) return -1;
    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    
    const fog::StateBatch4* batch = scheduler->get_arena().get_batch(batch_index);
    
    uint32_t white_dead = fog::kernel::batched_is_king_captured(batch, 0);
    uint32_t black_dead = fog::kernel::batched_is_king_captured(batch, 1);
    
    *out_mask = white_dead | black_dead;
    return 0;
}

} // extern "C"