#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "read_write_lock.hpp"

using namespace concurrency;

void stress_test() {
    ReadWriteLock rwlock;
    std::atomic<int> shared_data{0};
    std::atomic<bool> stop{false};
    
    // Multiple writers competing
    std::vector<std::thread> writers;
    for (int i = 0; i < 3; ++i) {
        writers.emplace_back([&, i]() {
            while (!stop.load()) {
                ReadWriteLock::WriteLockGuard guard(rwlock);
                shared_data = i;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (shared_data != i) {
                    std::cout << "VIOLATION: Writer " << i << " data corrupted!" << std::endl;
                }
            }
        });
    }
    
    // Multiple readers
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back([&]() {
            while (!stop.load()) {
                ReadWriteLock::ReadLockGuard guard(rwlock);
                int value = shared_data.load();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                // Value shouldn't change during read
                if (shared_data.load() != value) {
                    std::cout << "VIOLATION: Data changed during read!" << std::endl;
                }
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop = true;
    
    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();
    
    std::cout << "Stress test completed" << std::endl;
}

int main() {
    stress_test();
    return 0;
}