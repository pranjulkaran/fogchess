/**
 * @file src/runtime/scheduler.cpp
 * @brief Persistent Thread Pool Scheduler for zero-overhead SIMD job dispatch.
 */

#include "fog_arena.h"
#include "fog_command.h"
#include <vector>
#include <thread>
#include <atomic>

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
    explicit TaskScheduler(size_t max_batches) 
        : arena_(max_batches), shutdown_(false), generation_(0) {
        
        // Spin up one persistent worker per hardware CPU core
        uint32_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4; // Fallback safe default
        
        for (uint32_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&TaskScheduler::worker_loop, this);
        }
    }

    ~TaskScheduler() {
        // Broadcast shutdown signal to all spinning workers
        shutdown_.store(true, std::memory_order_release);
        generation_.fetch_add(1, std::memory_order_release); 
        
        for (auto& t : workers_) {
            if (t.joinable()) t.join();
        }
    }

    void step_all_parallel(const uint32_t* batch_ids, const uint32_t* commands, uint32_t count) {
        uint32_t num_batches = count / 4;
        
        // 1. Setup global work pointers (Strictly lock-free)
        current_batch_ids_ = batch_ids;
        current_commands_ = commands;
        total_batches_.store(num_batches, std::memory_order_relaxed);
        current_batch_idx_.store(0, std::memory_order_relaxed);
        completed_batches_.store(0, std::memory_order_relaxed);
        
        // 2. Broadcast the new generation to instantly wake all spinning workers
        generation_.fetch_add(1, std::memory_order_release);
        
        // 3. Main thread joins the work-stealing pool to help process tasks
        process_work();

        // 4. Barrier synchronization: Wait until every single batch is processed
        while (completed_batches_.load(std::memory_order_acquire) < num_batches) {
            std::this_thread::yield();
        }
    }

    StateArena& get_arena() { return arena_; }

private:
    StateArena arena_;
    
    // Persistent Worker Threads
    std::vector<std::thread> workers_;
    std::atomic<bool> shutdown_;
    
    // Lock-Free Synchronization State (Hardware cache-line aligned)
    alignas(64) std::atomic<uint32_t> generation_;
    alignas(64) std::atomic<uint32_t> current_batch_idx_;
    alignas(64) std::atomic<uint32_t> total_batches_;
    alignas(64) std::atomic<uint32_t> completed_batches_;
    
    const uint32_t* current_batch_ids_ = nullptr;
    const uint32_t* current_commands_ = nullptr;

    void process_work() {
        while (true) {
            // Atomic fetch-and-add to steal a batch index
            uint32_t b = current_batch_idx_.fetch_add(1, std::memory_order_acq_rel);
            
            if (b >= total_batches_.load(std::memory_order_relaxed)) {
                break; // No more work available for this step
            }
            
            // Execute kernel physics directly
            StateBatch4* batch = arena_.get_batch(current_batch_ids_[b]);
            fog::kernel::apply_batched_commands(batch, &current_commands_[b * 4]);
            
            // Mark batch as completed
            completed_batches_.fetch_add(1, std::memory_order_release);
        }
    }

    void worker_loop() {
        uint32_t local_gen = 0;
        while (!shutdown_.load(std::memory_order_acquire)) {
            uint32_t global_gen = generation_.load(std::memory_order_acquire);
            
            if (local_gen == global_gen) {
                // Spin-wait for new tasks to be broadcasted
                std::this_thread::yield();
                continue;
            }
            
            // Work is available! Process tasks until the queue is empty
            process_work();
            local_gen = global_gen;
        }
    }
};

} // namespace runtime
} // namespace fog