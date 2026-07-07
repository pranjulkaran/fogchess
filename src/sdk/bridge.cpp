/**
 * @file src/sdk/bridge.cpp
 * @brief C-ABI bridge translating external application calls into isolated runtime jobs.
 * @details Implements fog_sdk.h, keeping the underlying engine hidden behind opaque pointers.
 */

#include "fog_sdk.h"
#include "../runtime/scheduler.cpp"

extern "C" {

FogEngineHandle fog_engine_init(uint32_t max_batches) {
    try {
        // Instantiate the runtime system cleanly on the heap once
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

int32_t fog_batch_step(FogEngineHandle engine, const uint32_t* batch_indices, const uint16_t* commands, uint32_t count) {
    if (!engine || !batch_indices || !commands || count == 0) return -1;
    
    // Safety boundary check: must be a perfect multiple of 4 to exploit SIMD registers
    if (count % 4 != 0) return -2;

    auto* scheduler = reinterpret_cast<fog::runtime::TaskScheduler*>(engine);
    scheduler->step_all_parallel(batch_indices, commands, count);
    
    return 0; // Success code
}

}