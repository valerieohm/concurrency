#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <queue>

namespace concurrency {

/**
 * Producer-Consumer pattern with bounded buffer.
 * Demonstrates proper use of condition variables.
 */
template<typename T>
class ProducerConsumer {
public:
    explicit ProducerConsumer(size_t buffer_size) : buffer_size_(buffer_size) {}
    ~ProducerConsumer() {}

    
    /**
     * Start specified number of producer and consumer threads
     */
    void start(int num_producers, int num_consumers,
               std::function<T()> producer_func,
               std::function<void(T)> consumer_func) {
        running_.store(true);
        for (int i = 0; i < num_producers; i++) {
            producer_threads_.emplace_back(&ProducerConsumer::producer_worker,
                                           this, producer_func);
        }
        for (int i = 0; i < num_consumers; i++) {
            consumer_threads_.emplace_back(&ProducerConsumer::consumer_worker,
                                           this, consumer_func);
        }
    }

    void stop() {
        running_ = false;
        not_full_.notify_all();
        for (auto &thread : producer_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        producer_threads_.clear();
        not_empty_.notify_all();

        for (auto &thread : consumer_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        consumer_threads_.clear();
    }

    /**
     * Get statistics
     */
    size_t items_produced() const { return items_produced_.load(); }
    size_t items_consumed() const { return items_consumed_.load(); }
    size_t current_buffer_size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.size();
    }

private:
    size_t buffer_size_;
    // Bounded buffer implementation
    std::queue<T> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;   // Signals producers when space available
    std::condition_variable not_empty_;  // Signals consumers when items available
    
    // Thread management
    std::vector<std::thread> producer_threads_;
    std::vector<std::thread> consumer_threads_;
    
    // Statistics
    std::atomic<size_t> items_produced_{0};
    std::atomic<size_t> items_consumed_{0};
    std::atomic<bool> running_{false};
    
    // Worker functions
    void producer_worker(std::function<T()> producer_func) {
        while (running_.load()) {
            auto item = producer_func();
            
            {
            std::unique_lock<std::mutex> lock(mutex_);
                not_full_.wait(lock, [this]() { 
                    return buffer_.size() < buffer_size_ || !running_.load(); 
                });
                
                // Check again after wait - exit if stopped
                if (!running_.load()) {
                    break;
                }
                
                buffer_.push(item);
                items_produced_.fetch_add(1);  // Only increment after successful push
            }
            
            not_empty_.notify_one();
        }
    }
    void consumer_worker(std::function<void(T)> consumer_func) {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_.wait(lock, [this]() { return !buffer_.empty() || !running_.load(); });
            
            // Exit only if shutdown AND buffer is empty
            if (!running_.load() && buffer_.empty()) {
                lock.unlock();
                break;
            }
            
            auto item = buffer_.front();
            buffer_.pop();
            lock.unlock();
            
            items_consumed_.fetch_add(1);
            consumer_func(item);
            not_full_.notify_one();
        }
    }
};
} // namespace concurrency
// namespace concurrency