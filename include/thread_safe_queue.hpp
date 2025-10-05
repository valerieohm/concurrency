#ifndef THREAD_SAFE_QUEUE
#define THREAD_SAFE_QUEUE
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>
#include <atomic>
#include <stdexcept>

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
    void push(const T &item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.emplace(item);
        mCondVar.notify_one();
    }
    void push(T &&item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.emplace(std::move(item));
        mCondVar.notify_one();
    }

    /**
     * Remove and return an item from the queue.
     * Returns nullopt if queue is empty.
     * Should be thread-safe.
     */
    std::optional<T> try_pop() 
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mQueue.empty()) {
            T popped = std::move(mQueue.front());
            mQueue.pop();
            return popped;
        }
        return std::nullopt;
    }

    /**
     * Remove and return an item from the queue.
     * Blocks until an item is available.
     * Should be thread-safe and support proper cancellation.
     */
    T wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondVar.wait(lock, [this]{ return !mQueue.empty() || mShutdown.load(); });
        
        if (mShutdown.load() && mQueue.empty()) {
            throw std::runtime_error("Queue has been shut down");
        }
        
        T result = std::move(mQueue.front());
        mQueue.pop();
        return result;
    }

    /**
     * Check if queue is empty. Note: result may be stale immediately.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mQueue.empty();
    }

    /**
     * Get approximate size. Note: result may be stale immediately.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mQueue.size();
    }

    /**
     * Wake up all waiting threads (for shutdown scenarios)
     */
    void shutdown() {
        mShutdown = true;
        mCondVar.notify_all();
    }

private:
    // TODO: Implement with proper synchronization primitives
    mutable std::mutex mMutex;
    std::queue<T> mQueue;
    std::condition_variable mCondVar;
    std::atomic<bool> mShutdown = false;
};

} // namespace concurrency
#endif