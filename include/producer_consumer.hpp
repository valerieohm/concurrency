#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>

namespace concurrency {

/**
 * Producer-Consumer pattern with bounded buffer.
 * Demonstrates proper use of condition variables.
 */
template<typename T>
class ProducerConsumer {
public:
    explicit ProducerConsumer(size_t buffer_size);
    ~ProducerConsumer();
    
    /**
     * Start specified number of producer and consumer threads
     */
    void start(int num_producers, int num_consumers,
               std::function<T()> producer_func,
               std::function<void(T)> consumer_func);
    
    /**
     * Stop all threads gracefully
     */
    void stop();
    
    /**
     * Get statistics
     */
    size_t items_produced() const { return items_produced_; }
    size_t items_consumed() const { return items_consumed_; }
    size_t current_buffer_size() const;

private:
    size_t buffer_size_;
    std::atomic<size_t> items_produced_{0};
    std::atomic<size_t> items_consumed_{0};
    std::atomic<bool> running_{false};
    
    // TODO: Implement bounded buffer with proper synchronization
};

} // namespace concurrency