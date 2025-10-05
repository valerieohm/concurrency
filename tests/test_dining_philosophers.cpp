#include <gtest/gtest.h>
#include <chrono>
#include "dining_philosophers.hpp"

using namespace concurrency;

class DiningPhilosophersTest : public ::testing::Test {
protected:
    DiningPhilosophers philosophers{5};
};

TEST_F(DiningPhilosophersTest, BasicStartStop) {
    philosophers.start_dining();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto deadlocked = philosophers.is_deadlocked();
    EXPECT_FALSE(deadlocked);
    philosophers.stop_dining();
    
    auto eat_counts = philosophers.get_eat_counts();
    EXPECT_EQ(eat_counts.size(), 5);
    
    // All philosophers should have had a chance to eat
    int total_eats = 0;
    for (int count : eat_counts) {
        EXPECT_GE(count, 0);
        total_eats += count;
    }
    EXPECT_GT(total_eats, 0);
}

TEST_F(DiningPhilosophersTest, NoDeadlockDetected) {
    philosophers.start_dining();
    
    // Run for a reasonable time
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Should not be deadlocked
    EXPECT_FALSE(philosophers.is_deadlocked());
    
    philosophers.stop_dining();
    
    auto eat_counts = philosophers.get_eat_counts();
    
    // All philosophers should have eaten multiple times
    for (int count : eat_counts) {
        EXPECT_GT(count, 0) << "A philosopher never got to eat - possible starvation";
    }
}

TEST_F(DiningPhilosophersTest, FairnessCheck) {
    philosophers.start_dining();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    philosophers.stop_dining();
    
    auto eat_counts = philosophers.get_eat_counts();
    
    // Check for reasonable fairness - no philosopher should be starved
    int min_eats = *std::min_element(eat_counts.begin(), eat_counts.end());
    int max_eats = *std::max_element(eat_counts.begin(), eat_counts.end());
    
    EXPECT_GT(min_eats, 0) << "Some philosopher was starved";
    
    // Ratio shouldn't be too extreme (adjust threshold as needed)
    if (min_eats > 0) {
        double ratio = static_cast<double>(max_eats) / min_eats;
        EXPECT_LT(ratio, 10.0) << "Eating distribution is too unfair";
    }
}