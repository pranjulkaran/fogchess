/**
 * @file src/kernel/hasher.hpp
 * @brief Deterministic hashing algorithm for replay validation.
 */

#ifndef FOG_HASHER_HPP
#define FOG_HASHER_HPP

#include "fog_state.h"

namespace fog {
namespace kernel {

constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

/**
 * @brief Hashes a single isolated game lane into a deterministic 64-bit fingerprint.
 */
inline uint64_t hash_lane(const StateBatch4* batch, int lane) {
    uint64_t hash = FNV_OFFSET_BASIS;

    // Hash the 12 piece bitboards
    for (int c = 0; c < 2; ++c) {
        for (int p = 0; p < 6; ++p) {
            uint64_t bits = batch->pieces[c][p][lane];
            hash ^= bits;
            hash *= FNV_PRIME;
        }
    }

    // Hash the game metadata and turn
    hash ^= batch->metadata[lane];
    hash *= FNV_PRIME;
    
    hash ^= batch->active_turn[lane];
    hash *= FNV_PRIME;

    return hash;
}

} // namespace kernel
} // namespace fog

#endif // FOG_HASHER_HPP