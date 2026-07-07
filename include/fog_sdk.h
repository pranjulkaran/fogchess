/**
 * @file include/fog_sdk.h
 * @brief Flat C-ABI defining the strict boundary between Applications and FogEngine.
 * @details This is the ONLY file that Python, C#, or Rust wrappers will ever see. 
 *          It completely hides the C++ object orientation and memory management, 
 *          exposing only simple primitives and opaque pointers.
 */

#ifndef FOG_SDK_H
#define FOG_SDK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief Opaque handle representing a running instance of the engine runtime. 
 */
typedef void* FogEngineHandle;

/**
 * @brief Boots the engine runtime and allocates the global Memory Arena.
 * @param max_batches Maximum number of 4-game batches the arena can hold.
 * @return FogEngineHandle Opaque pointer to the instantiated engine.
 */
FogEngineHandle fog_engine_init(uint32_t max_batches);

/**
 * @brief Gracefully shuts down the engine and frees the arena memory.
 * @param engine The target engine instance.
 */
void fog_engine_shutdown(FogEngineHandle engine);

/**
 * @brief Submits a flat array of commands to step multiple games simultaneously.
 * @param engine The target engine instance.
 * @param batch_indices Array of active game IDs being stepped (size = count).
 * @param commands Array of 16-bit packed command payloads (size = count).
 * @param count The number of parallel environments to step.
 * @return int32_t Returns 0 on success, or a negative error code on failure.
 */
int32_t fog_batch_step(FogEngineHandle engine, const uint32_t* batch_indices, const uint32_t* commands, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif // FOG_SDK_H