#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "thread_safe_queue.hpp"

using namespace concurrency;

class ThreadSafeQueueTest : public ::testing::Test {
protected:
    ThreadSafeQueue<int> queue;
};

TEST_F(ThreadSafeQueueTest, BasicPushPop) {
    EXPECT_TRUE(queue.empty());
    
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
    
    auto result = queue.try_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, TryPopOnEmpty) {
    auto result = queue.try_pop();
    EXPECT_FALSE(result.has_value());
}

TEST_F(ThreadSafeQueueTest, MultipleProducersConsumers) {
    const int num_producers = 4;
    const int num_consumers = 3;
    const int items_per_producer = 100;
    
    std::atomic<int> total_consumed{0};
    std::vector<std::thread> threads;
    
    // Start producers
    for (int i = 0; i < num_producers; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                queue.push(i * items_per_producer + j);
            }
        });
    }
    
    // Start consumers
    for (int i = 0; i < num_consumers; ++i) {
        threads.emplace_back([&]() {
            while (total_consumed.load() < num_producers * items_per_producer) {
                auto item = queue.try_pop();
                if (item.has_value()) {
                    total_consumed.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_consumed.load(), num_producers * items_per_producer);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, WaitAndPopBlocking) {
    std::atomic<bool> consumer_done{false};
    int consumed_value = -1;
    
    std::thread consumer([&]() {
        consumed_value = queue.wait_and_pop();
        consumer_done = true;
    });
    
    // Consumer should be blocked
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(consumer_done.load());
    
    // Produce an item
    queue.push(123);
    
    consumer.join();
    EXPECT_TRUE(consumer_done.load());
    EXPECT_EQ(consumed_value, 123);
}

TEST_F(ThreadSafeQueueTest, ShutdownWakesWaitingThreads) {
    std::atomic<int> threads_woken{0};
    std::vector<std::thread> waiting_threads;
    
    // Start multiple threads waiting
    for (int i = 0; i < 3; ++i) {
        waiting_threads.emplace_back([&]() {
            try {
                queue.wait_and_pop();
            } catch (...) {
                // Expected when shutdown
            }
            threads_woken.fetch_add(1);
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.shutdown();
    
    for (auto& t : waiting_threads) {
        t.join();
    }
    
    EXPECT_EQ(threads_woken.load(), 3);
}