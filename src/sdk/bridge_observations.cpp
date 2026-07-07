/**
 * @file src/sdk/bridge_observations.cpp
 * @brief Flattens the observation generation into the C-ABI.
 */

#include "fog_sdk.h"
#include "fog_arena.h"

// Forward declarations to link with the kernel logic
namespace fog {
namespace kernel {
void generate_layer3_tensor_lane(const StateBatch4* batch, int lane,
                                 uint64_t visible_mask,
                                 const int32_t* scouted_sqs, int scouted_count,
                                 const int32_t* ghost_sqs, int ghost_count,
                                 bool is_scout_phase,
                                 float* out_buffer);

void generate_true_state_matrix_lane(const StateBatch4* batch, int lane, int8_t* out_buffer);
}
}

extern "C" {

int32_t fog_get_layer3_tensor(FogEngineHandle engine, 
                              uint32_t batch_index, 
                              uint32_t lane,
                              uint64_t visible_mask, 
                              const int32_t* scouted_sqs, int32_t scouted_count,
                              const int32_t* ghost_sqs, int32_t ghost_count,
                              int32_t is_scout_phase,
                              float* out_buffer) {
    if (!engine || !out_buffer) return -1;
    
    // Cast the opaque C-handle back into the Arena allocator[cite: 10, 12]
    auto* arena = static_cast<fog::runtime::StateArena*>(engine);
    
    // Grab the direct pointer to the SIMD-aligned batch[cite: 10]
    const fog::StateBatch4* batch = arena->get_batch(batch_index);

    fog::kernel::generate_layer3_tensor_lane(batch, lane, visible_mask,
                                             scouted_sqs, scouted_count,
                                             ghost_sqs, ghost_count,
                                             (is_scout_phase != 0), out_buffer);
    return 0;
}

int32_t fog_get_true_state_matrix(FogEngineHandle engine, 
                                  uint32_t batch_index, 
                                  uint32_t lane, 
                                  int8_t* out_buffer) {
    if (!engine || !out_buffer) return -1;
    
    auto* arena = static_cast<fog::runtime::StateArena*>(engine);
    const fog::StateBatch4* batch = arena->get_batch(batch_index);

    fog::kernel::generate_true_state_matrix_lane(batch, lane, out_buffer);
    return 0;
}

} // extern "C"