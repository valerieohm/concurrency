#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

namespace concurrency {

/**
 * A thread-safe queue implementation.
 * Should support multiple producers and consumers safely.
 */
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;
    
    // Non-copyable, non-movable for simplicity
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    
    /**
     * Add an item to the queue. Should be thread-safe.
     */
    void push(const T& item);
    void push(T&& item);
    
    /**
     * Remove and return an item from the queue.
     * Returns nullopt if queue is empty.
     * Should be thread-safe.
     */
    std::optional<T> try_pop();
    
    /**
     * Remove and return an item from the queue.
     * Blocks until an item is available.
     * Should be thread-safe and support proper cancellation.
     */
    T wait_and_pop();
    
    /**
     * Check if queue is empty. Note: result may be stale immediately.
     */
    bool empty() const;
    
    /**
     * Get approximate size. Note: result may be stale immediately.
     */
    size_t size() const;
    
    /**
     * Wake up all waiting threads (for shutdown scenarios)
     */
    void shutdown();

private:
    // TODO: Implement with proper synchronization primitives
    std::mutex mMutex;
    std::queue<T> mQueue;
};

} // namespace concurrency