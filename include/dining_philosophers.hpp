#pragma once
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

namespace concurrency {

/**
 * Classic dining philosophers problem.
 * Implement a deadlock-free solution.
 */
class DiningPhilosophers {
public:
    explicit DiningPhilosophers(int num_philosophers = 5);
    ~DiningPhilosophers();
    
    /**
     * Start the dining session. Each philosopher should:
     * 1. Think for a random time
     * 2. Try to pick up both forks
     * 3. Eat for a random time  
     * 4. Put down both forks
     * 5. Repeat until stopped
     */
    void start_dining();
    
    /**
     * Stop all philosophers
     */
    void stop_dining();
    
    /**
     * Get statistics about how many times each philosopher has eaten
     */
    std::vector<int> get_eat_counts() const;
    
    /**
     * Check if any deadlock has been detected
     */
    bool is_deadlocked() const;

private:
    int num_philosophers_;
    std::atomic<bool> running_{false};
    
    // TODO: Implement proper fork management and deadlock prevention
    void philosopher_routine(int philosopher_id);
};

} // namespace concurrency