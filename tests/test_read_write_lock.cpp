#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "read_write_lock.hpp"

using namespace concurrency;

class ReadWriteLockTest : public ::testing::Test {
protected:
    ReadWriteLock rwlock;
    std::atomic<int> shared_data{0};
    std::atomic<int> concurrent_readers{0};
    std::atomic<bool> writer_active{false};
};

TEST_F(ReadWriteLockTest, BasicReadLock) {
    {
        ReadWriteLock::ReadLockGuard guard(rwlock);
        // Should acquire successfully
        EXPECT_EQ(shared_data.load(), 0);
    }
    // Lock should be released
}

TEST_F(ReadWriteLockTest, BasicWriteLock) {
    {
        ReadWriteLock::WriteLockGuard guard(rwlock);
        shared_data = 42;
        EXPECT_EQ(shared_data.load(), 42);
    }
    // Lock should be released
}

TEST_F(ReadWriteLockTest, MultipleReaders) {
    const int num_readers = 5;
    std::vector<std::thread> readers;
    std::atomic<int> max_concurrent{0};
    
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            ReadWriteLock::ReadLockGuard guard(rwlock);
            
            int current = concurrent_readers.fetch_add(1) + 1;
            int current_max = max_concurrent.load();
            while (current > current_max && 
                   !max_concurrent.compare_exchange_weak(current_max, current)) {
                current_max = max_concurrent.load();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            concurrent_readers.fetch_sub(1);
        });
    }
    
    for (auto& t : readers) {
        t.join();
    }
    
    // Should have allowed multiple concurrent readers
    EXPECT_GT(max_concurrent.load(), 1);
    EXPECT_EQ(concurrent_readers.load(), 0);
}

TEST_F(ReadWriteLockTest, WriterExcludesReaders) {
    std::atomic<bool> reader_started{false};
    std::atomic<bool> reader_completed{false};
    std::atomic<bool> writer_completed{false};
    
    std::thread writer([&]() {
        ReadWriteLock::WriteLockGuard guard(rwlock);
        writer_active = true;
        
        // Wait for reader to start (and be blocked)
        while (!reader_started.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Reader should still be blocked
        EXPECT_FALSE(reader_completed.load());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        writer_active = false;
        writer_completed = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::thread reader([&]() {
        reader_started = true;
        ReadWriteLock::ReadLockGuard guard(rwlock);
        
        // Should only acquire after writer completes
        EXPECT_TRUE(writer_completed.load());
        EXPECT_FALSE(writer_active.load());
        reader_completed = true;
    });
    
    writer.join();
    reader.join();
    
    EXPECT_TRUE(writer_completed.load());
    EXPECT_TRUE(reader_completed.load());
}

TEST_F(ReadWriteLockTest, ReaderExcludesWriter) {
    std::atomic<bool> reader_acquired{false};
    std::atomic<bool> writer_started{false};
    std::atomic<bool> writer_completed{false};
    
    std::thread reader([&]() {
        ReadWriteLock::ReadLockGuard guard(rwlock);
        reader_acquired = true;
        
        // Wait for writer to start (and be blocked)
        while (!writer_started.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Writer should be blocked
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_FALSE(writer_completed.load());
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::thread writer([&]() {
        writer_started = true;
        ReadWriteLock::WriteLockGuard guard(rwlock);
        
        // Should only acquire after reader completes
        EXPECT_TRUE(reader_acquired.load());
        writer_completed = true;
    });
    
    reader.join();
    writer.join();
    
    EXPECT_TRUE(writer_completed.load());
}