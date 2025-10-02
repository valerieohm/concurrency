#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include "producer_consumer.hpp"

using namespace concurrency;

class ProducerConsumerTest : public ::testing::Test {
protected:
    ProducerConsumer<int> pc{10}; // buffer size 10
};

TEST_F(ProducerConsumerTest, BasicProducerConsumer) {
    std::atomic<int> next_item{0};
    std::atomic<int> sum_consumed{0};
    
    auto producer_func = [&]() -> int {
        return next_item.fetch_add(1);
    };
    
    auto consumer_func = [&](int item) {
        sum_consumed.fetch_add(item);
    };
    
    pc.start(1, 1, producer_func, consumer_func);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pc.stop();
    
    EXPECT_GT(pc.items_produced(), 0);
    EXPECT_GT(pc.items_consumed(), 0);
    EXPECT_EQ(pc.items_produced(), pc.items_consumed());
}

TEST_F(ProducerConsumerTest, MultipleProducersConsumers) {
    std::atomic<int> next_item{0};
    std::atomic<int> items_consumed_count{0};
    
    auto producer_func = [&]() -> int {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        return next_item.fetch_add(1);
    };
    
    auto consumer_func = [&](int item) {
        std::this_thread::sleep_for(std::chrono::microseconds(150));
        items_consumed_count.fetch_add(1);
    };
    
    pc.start(3, 2, producer_func, consumer_func);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pc.stop();
    
    EXPECT_GT(pc.items_produced(), 10);
    EXPECT_GT(pc.items_consumed(), 5);
    EXPECT_EQ(pc.items_consumed(), items_consumed_count.load());
}

TEST_F(ProducerConsumerTest, BufferBounds) {
    std::atomic<bool> buffer_full_detected{false};
    std::atomic<int> max_buffer_size{0};
    
    auto producer_func = [&]() -> int {
        // Fast producer
        return 42;
    };
    
    auto consumer_func = [&](int item) {
        // Slow consumer
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Check buffer size
        size_t current_size = pc.current_buffer_size();
        int current_max = max_buffer_size.load();
        while (current_size > current_max && 
               !max_buffer_size.compare_exchange_weak(current_max, current_size)) {
            current_max = max_buffer_size.load();
        }
        
        if (current_size >= 10) {
            buffer_full_detected = true;
        }
    };
    
    pc.start(2, 1, producer_func, consumer_func);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pc.stop();
    
    // Buffer should have reached capacity
    EXPECT_TRUE(buffer_full_detected.load());
    EXPECT_LE(max_buffer_size.load(), 10); // Should not exceed buffer size
}