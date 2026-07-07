/**
 * @file include/fog_sdk.h
 * @brief Updated Flat C-ABI
 */
#ifndef FOG_SDK_H
#define FOG_SDK_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* FogEngineHandle;

FogEngineHandle fog_engine_init(uint32_t max_batches);
void fog_engine_shutdown(FogEngineHandle engine);

// Standard Batch Step
int32_t fog_batch_step(FogEngineHandle engine, const uint32_t* batch_indices, const uint32_t* commands, uint32_t count);

// [NEW] Resets a specific list of batches back to starting positions
int32_t fog_batch_reset(FogEngineHandle engine, const uint32_t* batch_indices, uint32_t count);

// [NEW] Generates pseudo-legal commands for a specific lane in a batch
// out_commands must point to an array of at least 256 uint32_t elements (Matches MAX_MOVES)[cite: 4]
int32_t fog_get_moves(FogEngineHandle engine, uint32_t batch_index, uint32_t lane, uint32_t* out_commands, uint32_t* out_count);

// [NEW] Returns a bitmask representing which lanes in a batch hit terminal conditions (King captured)
int32_t fog_check_terminations(FogEngineHandle engine, uint32_t batch_index, uint32_t* out_mask);

#ifdef __cplusplus
}
#endif
#endif // FOG_SDK_H