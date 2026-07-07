/**
 * @file src/runtime/scheduler.cpp
 * @brief High-speed runtime task orchestrator grouping incoming steps into SIMD jobs.
 */

#include "fog_arena.h"
#include "fog_command.h"
#include <vector>
#include <thread>
#include <future>

// Forward declaration bridging the Kernel's rules engine into the Runtime layer
namespace fog {
namespace kernel {
    void apply_batched_commands(StateBatch4* batch, const uint32_t* commands);
}
}

namespace fog {
namespace runtime {

class TaskScheduler {
public:
    explicit TaskScheduler(size_t max_batches) : arena_(max_batches) {}

    /**
     * @brief Steps a high-throughput array of commands by grouping them into batches of 4.
     */
    void step_all_parallel(const uint32_t* batch_ids, const uint32_t* commands, uint32_t count) {
        uint32_t num_batches = count / 4;
        std::vector<std::future<void>> workers;

        // Spin up asynchronous tasks for each batch
        for (uint32_t b = 0; b < num_batches; ++b) {
            workers.push_back(std::async(std::launch::async, [this, b, batch_ids, commands]() {
                // Check out the 4-game block from the zero-allocation arena
                StateBatch4* batch = arena_.get_batch(batch_ids[b]);
                
                // Delegate the rule execution and SIMD occupancy updates entirely to the Kernel
                fog::kernel::apply_batched_commands(batch, &commands[b * 4]);
            }));
        }

        // Barrier synchronization point: Wait for all parallel batches to finish computing
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