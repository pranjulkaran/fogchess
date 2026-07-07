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

// [NEW] Exports the exact 1:1 Layer 3 observation tensor [14 x 8 x 8] for a specific SIMD lane
// out_buffer must point to a pre-allocated flat float array of size 896
int32_t fog_get_layer3_tensor(FogEngineHandle engine, 
                              uint32_t batch_index, 
                              uint32_t lane,
                              uint64_t visible_mask, 
                              const int32_t* scouted_sqs, int32_t scouted_count,
                              const int32_t* ghost_sqs, int32_t ghost_count,
                              int32_t is_scout_phase,
                              float* out_buffer);

// [NEW] Exports the True State Matrix (8x8 dense representation) for a specific SIMD lane
// out_buffer must point to a pre-allocated flat int8_t array of size 64
int32_t fog_get_true_state_matrix(FogEngineHandle engine, 
                                  uint32_t batch_index, 
                                  uint32_t lane, 
                                  int8_t* out_buffer);

#ifdef __cplusplus
}
#endif
#endif // FOG_SDK_H