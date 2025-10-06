#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include "resource_pool.hpp"

using namespace concurrency;

// Mock resource for testing
struct MockConnection {
    int id;
    std::atomic<int> usage_count{0};
    
    explicit MockConnection(int connection_id) : id(connection_id) {}
    
    void use() {
        usage_count.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

class ResourcePoolTest : public ::testing::Test {
protected:
    ResourcePool<MockConnection> pool{3};  // Pool of 3 connections
};

TEST_F(ResourcePoolTest, BasicAcquireRelease) {
    // Test basic acquire/release cycle
    auto resource = pool.acquire();
    ASSERT_TRUE(resource != nullptr);
    EXPECT_EQ(pool.current_usage(), 1);
    EXPECT_EQ(pool.available_count(), 2);
    
    pool.release(resource);
    EXPECT_EQ(pool.current_usage(), 0);
    EXPECT_EQ(pool.available_count(), 3);
}

TEST_F(ResourcePoolTest, RAIIWrapper) {
    // Test RAII automatic release
    {
        ResourceGuard<MockConnection> guard(pool);
        ASSERT_TRUE(guard.valid());
        EXPECT_EQ(pool.current_usage(), 1);
        
        guard->use();  // Use the resource
        EXPECT_GT(guard->usage_count.load(), 0);
    } // Automatic release here
    
    EXPECT_EQ(pool.current_usage(), 0);
}

TEST_F(ResourcePoolTest, PoolExhaustion) {
    // Acquire all resources
    std::vector<std::shared_ptr<MockConnection>> resources;
    
    for (int i = 0; i < 3; ++i) {
        auto resource = pool.acquire();
        ASSERT_TRUE(resource != nullptr);
        resources.push_back(resource);
    }
    
    EXPECT_EQ(pool.current_usage(), 3);
    EXPECT_EQ(pool.available_count(), 0);
    
    // Try to acquire with timeout (should fail)
    auto timeout_resource = pool.try_acquire(std::chrono::milliseconds(100));
    EXPECT_FALSE(timeout_resource);
    
    // Release one resource
    pool.release(resources[0]);
    resources[0] = nullptr;
    
    // Now acquisition should succeed
    auto new_resource = pool.try_acquire(std::chrono::milliseconds(100));
    EXPECT_TRUE(new_resource != nullptr);
    
    // Clean up
    for (auto& resource : resources) {
        if (resource) pool.release(resource);
    }
    pool.release(new_resource);
}

TEST_F(ResourcePoolTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    std::atomic<int> timeout_operations{0};
    
    // Launch multiple threads competing for resources
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                // Try to acquire with short timeout
                auto resource = pool.try_acquire(std::chrono::milliseconds(50));
                
                if (resource) {
                    successful_operations.fetch_add(1);
                    
                    // Simulate work
                    resource->use();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    
                    pool.release(resource);
                } else {
                    timeout_operations.fetch_add(1);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify statistics
    EXPECT_EQ(successful_operations.load() + timeout_operations.load(), 
              num_threads * operations_per_thread);
    EXPECT_GT(successful_operations.load(), 0);
    EXPECT_EQ(pool.current_usage(), 0);  // All resources should be released
    EXPECT_LE(pool.peak_usage(), 3);     // Should never exceed pool size
    
    std::cout << "Successful operations: " << successful_operations.load() << std::endl;
    std::cout << "Timeout operations: " << timeout_operations.load() << std::endl;
    std::cout << "Peak usage: " << pool.peak_usage() << std::endl;
}

TEST_F(ResourcePoolTest, SemaphoreBlocking) {
    std::atomic<bool> thread_started{false};
    std::atomic<bool> resource_acquired{false};
    
    // Acquire all resources in main thread
    std::vector<std::shared_ptr<MockConnection>> resources;
    for (int i = 0; i < 3; ++i) {
        resources.push_back(pool.acquire());
    }
    
    // Start a thread that will block on semaphore
    std::thread blocking_thread([&]() {
        thread_started = true;
        auto resource = pool.acquire();  // This will block
        resource_acquired = true;
        pool.release(resource);
    });
    
    // Wait for thread to start and attempt acquisition
    while (!thread_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Give thread time to block on semaphore
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(resource_acquired.load());  // Should still be blocked
    
    // Release one resource - should unblock the waiting thread
    pool.release(resources[0]);
    
    // Wait for thread to complete
    blocking_thread.join();
    EXPECT_TRUE(resource_acquired.load());  // Should now be acquired
    
    // Clean up remaining resources
    for (size_t i = 1; i < resources.size(); ++i) {
        pool.release(resources[i]);
    }
}