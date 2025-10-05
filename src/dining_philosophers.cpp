#include "dining_philosophers.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

namespace concurrency {

DiningPhilosophers::DiningPhilosophers(int num_philosophers)
    : num_philosophers_(num_philosophers), forks_(num_philosophers),
      eat_counts_(num_philosophers), states_(num_philosophers) {
    threads_.reserve(num_philosophers);
    running_.store(false);
    // TODO: Initialize forks and other data structures
}

DiningPhilosophers::~DiningPhilosophers() {
    if (running_.load()) {
        stop_dining();
    }
}

void DiningPhilosophers::start_dining() {
    if (running_.exchange(true)) {
        return; // Already running
    }

    // TODO: Initialize philosopher threads
    for (int i = 0; i < num_philosophers_; ++i) {
        // Create philosopher thread
        threads_.emplace_back(&DiningPhilosophers::philosopher_routine, this,
                              i);
    }
}

void DiningPhilosophers::stop_dining() {
    running_ = false;
    if (is_deadlocked()) {
        throw;
    }
    // TODO: Join all philosopher threads
    for (auto &thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}

std::vector<int> DiningPhilosophers::get_eat_counts() const {
    std::vector<int> res(eat_counts_.size());
    std::transform(eat_counts_.begin(), eat_counts_.end(), res.begin(),
                   [](std::atomic<int> const &i) { return i.load(); });
    return res;
}

bool DiningPhilosophers::is_deadlocked() const {
    // TODO: Implement deadlock detection
    State state = states_[0].load();
    if ((state == State::Eat) || (state == State::Think))
        return false;
    bool res =
        std::all_of(states_.begin(), states_.end(),
                    [state](auto const &s) { return s.load() == state; });
    return res;
}

void DiningPhilosophers::philosopher_routine(int philosopher_id) {
    std::mt19937 gen(philosopher_id + std::time(nullptr));
    std::uniform_int_distribution<> think_time(10, 100);
    std::uniform_int_distribution<> eat_time(10, 50);

    auto minfork = [](int id, int total) {
        return std::min(id, (id + 1) % total);
    };
    auto maxfork = [](int id, int total) {
        return std::max(id, (id + 1) % total);
    };
    while (running_.load()) {
        // Think
        states_[philosopher_id].store(State::Think);
        // std::this_thread::sleep_for(std::chrono::milliseconds(think_time(gen)));

        {
            states_[philosopher_id].store(State::WaitLeft);

            output(philosopher_id, "wait for left fork");
            std::lock_guard<std::mutex> left_fork(
                forks_[minfork(philosopher_id, num_philosophers_)]);
            states_[philosopher_id].store(State::WaitRight);

            output(philosopher_id, "wait for right fork");
            std::lock_guard<std::mutex> right_fork(
                forks_[maxfork(philosopher_id, num_philosophers_)]);
            states_[philosopher_id].store(State::Eat);

            std::this_thread::sleep_for(
                std::chrono::milliseconds(eat_time(gen)));
            eat_counts_[philosopher_id].fetch_add(1);
        }
        output(philosopher_id, "finished eating");
    }
}
void DiningPhilosophers::output(int id, std::string const &msg) {
    return;
    // std::lock_guard<std::mutex> lock(output_mutex_);
    // std::cout << id << " : " << msg << std::endl;
}

} // namespace concurrency