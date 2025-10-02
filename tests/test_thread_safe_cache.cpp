#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <string>
#include "thread_safe_cache.hpp"

using namespace concurrency;

class ThreadSafeCacheTest : public ::testing::Test {
protected:
    ThreadSafeCache<std::string, int> cache{3}; // capacity 3
};

TEST_F(ThreadSafeCacheTest, BasicPutGet) {
    EXPECT_EQ(cache.size(), 0);
    EXPECT_FALSE(cache.contains("key1"));
    
    cache.put("key1", 100);
    EXPECT_EQ(cache.size(), 1);
    EXPECT_TRUE(cache.contains("key1"));
    
    auto result = cache.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 100);
}

TEST_F(ThreadSafeCacheTest, GetNonExistent) {
    auto result = cache.get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ThreadSafeCacheTest, LRUEviction) {
    // Fill cache to capacity
    cache.put("key1", 1);
    cache.put("key2", 2);
    cache.put("key3", 3);
    EXPECT_EQ(cache.size(), 3);
    
    // Access key1 to make it recently used
    cache.get("key1");
    
    // Add new item - should evict key2 (least recently used)
    cache.put("key4", 4);
    EXPECT_EQ(cache.size(), 3);
    
    EXPECT_TRUE(cache.contains("key1"));
    EXPECT_FALSE(cache.contains("key2")); // evicted
    EXPECT_TRUE(cache.contains("key3"));
    EXPECT_TRUE(cache.contains("key4"));
}

TEST_F(ThreadSafeCacheTest, ConcurrentReads) {
    // Pre-populate cache
    for (int i = 0; i < 3; ++i) {
        cache.put("key" + std::to_string(i), i * 10);
    }
    
    const int num_readers = 10;
    const int reads_per_reader = 100;
    std::atomic<int> successful_reads{0};
    std::vector<std::thread> readers;
    
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            for (int j = 0; j < reads_per_reader; ++j) {
                std::string key = "key" + std::to_string(j % 3);
                auto result = cache.get(key);
                if (result.has_value()) {
                    successful_reads.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& t : readers) {
        t.join();
    }
    
    // All reads should succeed since keys exist
    EXPECT_EQ(successful_reads.load(), num_readers * reads_per_reader);
}

TEST_F(ThreadSafeCacheTest, ConcurrentReadWrites) {
    std::atomic<bool> stop{false};
    std::atomic<int> operations_completed{0};
    std::vector<std::thread> threads;
    
    // Writer thread
    threads.emplace_back([&]() {
        int counter = 0;
        while (!stop.load()) {
            cache.put("key" + std::to_string(counter % 5), counter);
            operations_completed.fetch_add(1);
            ++counter;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&]() {
            while (!stop.load()) {
                for (int j = 0; j < 5; ++j) {
                    cache.get("key" + std::to_string(j));
                    operations_completed.fetch_add(1);
                }
                std::this_thread::yield();
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(operations_completed.load(), 0);
    // Verify cache is still in valid state
    EXPECT_LE(cache.size(), 3);
}