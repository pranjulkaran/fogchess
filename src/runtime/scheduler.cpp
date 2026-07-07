/**
 * @file src/runtime/scheduler.cpp
 * @brief High-speed runtime task orchestrator grouping incoming steps into SIMD jobs.
 */

#include "fog_arena.h"
#include "fog_command.h"
#include <vector>
#include <thread>
#include <future>

// Forward declaration bridging the Kernel's math function into the Runtime layer
namespace fog {
namespace kernel {
    void update_batched_occupancy(StateBatch4* batch);
}
}

namespace fog {
namespace runtime {

class TaskScheduler {
public:
    explicit TaskScheduler(size_t max_batches) : arena_(max_batches) {}

    /**
     * @brief Steps a high-throughput array of commands by grouping them into batches of 4.
     * @note Upgraded to const uint32_t* to match the 32-bit Command payload.
     */
    void step_all_parallel(const uint32_t* batch_ids, const uint32_t* commands, uint32_t count) {
        uint32_t num_batches = count / 4;
        std::vector<std::future<void>> workers;

        for (uint32_t b = 0; b < num_batches; ++b) {
            workers.push_back(std::async(std::launch::async, [this, b, batch_ids, commands]() {
                StateBatch4* batch = arena_.get_batch(batch_ids[b]);
                
                for (int lane = 0; lane < 4; ++lane) {
                    uint32_t flat_idx = (b * 4) + lane;
                    Command cmd(commands[flat_idx]);
                    
                    uint64_t from_mask = ~(1ULL << cmd.get_from_square());
                    uint64_t to_mask = (1ULL << cmd.get_to_square());
                    
                    batch->pieces[batch->active_turn[lane]][cmd.get_piece_type()][lane] &= from_mask;
                    batch->pieces[batch->active_turn[lane]][cmd.get_piece_type()][lane] |= to_mask;
                }
                
                // Triggers the AVX2 calculation via the forward declaration
                fog::kernel::update_batched_occupancy(batch);
            }));
        }

        for (auto& task : workers) {
            task.wait();
        }
    }

    StateArena& get_arena() { return arena_; }

private:
    StateArena arena_;
};

} // namespace runtime
} // namespace fog