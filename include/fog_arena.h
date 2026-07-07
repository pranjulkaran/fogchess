#ifndef FOG_ARENA_H
#define FOG_ARENA_H

#include "fog_state.h"
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace fog {
namespace runtime {

/**
 * A pre-allocated memory pool for batched game states.
 * Guarantees zero heap allocations during the simulation tick.
 */
class StateArena {
public:
    // Initialize the arena with a fixed maximum capacity (e.g., 10,000 batches)
    explicit StateArena(size_t max_batches) 
        : capacity_(max_batches), head_(0) {
        
        // Allocate the entire memory block up front.
        // Cache-aligned at 64-bytes to ensure safe SIMD loads.
        memory_pool_.resize(max_batches);
        
        // Initialize the free list
        free_indices_.reserve(max_batches);
        for (size_t i = 0; i < max_batches; ++i) {
            free_indices_.push_back(max_batches - 1 - i);
        }
    }

    // O(1) allocation: Pop an available index instead of calling 'new'
    size_t allocate_batch() {
        if (free_indices_.empty()) {
            throw std::runtime_error("StateArena Out of Memory: Max capacity reached.");
        }
        size_t index = free_indices_.back();
        free_indices_.pop_back();
        return index;
    }

    // O(1) deallocation: Push the index back to the free list instead of calling 'delete'
    void free_batch(size_t index) {
        free_indices_.push_back(index);
    }

    // Get a direct pointer to the SIMD-aligned batch memory
    StateBatch4* get_batch(size_t index) {
        return &memory_pool_[index];
    }

private:
    size_t capacity_;
    size_t head_;
    
    // Contiguous memory block holding the actual data
    alignas(64) std::vector<StateBatch4> memory_pool_;
    
    // Stack of available indices
    std::vector<size_t> free_indices_;
};

} // namespace runtime
} // namespace fog

#endif // FOG_ARENA_H