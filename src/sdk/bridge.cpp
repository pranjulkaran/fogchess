/**
 * @file src/sdk/bridge.cpp
 * @brief C-ABI bridge translating external application calls into isolated runtime jobs.
 */

#include "fog_sdk.h"
#include "../runtime/scheduler.cpp" // Includes the scheduler directly for the compilation unit

extern "C" {

FogEngineHandle fog_engine_init(uint32_t max_batches) {
    try {
        auto* scheduler = new fog::runtime::TaskScheduler(max_batches);
        return reinterpret_cast<FogEngineHandle>(scheduler);
    } catch (...) {
        return nullptr;
    }
}

void fog_engine_shutdown(FogEngineHandle engine) {
    if (engine) {
        auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
        delete scheduler;
    }
}

// Ensure commands is explicitly uint32_t* here
int32_t fog_batch_step(FogEngineHandle engine, const uint32_t* batch_indices, const uint32_t* commands, uint32_t count) {
    if (!engine || !batch_indices || !commands || count == 0) return -1;
    if (count % 4 != 0) return -2;

    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    scheduler->step_all_parallel(batch_indices, commands, count);
    
    return 0; 
}

}