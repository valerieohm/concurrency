#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>

namespace concurrency {

/**
 * Custom read-write lock implementation.
 * Multiple readers OR one writer at a time.
 * Writers have priority over readers.
 */
class ReadWriteLock {
public:
    ReadWriteLock() = default;
    ~ReadWriteLock() = default;

    ReadWriteLock(const ReadWriteLock &) = delete;
    ReadWriteLock &operator=(const ReadWriteLock &) = delete;

    /**
     * Acquire read lock. Multiple readers allowed simultaneously.
     * Should block if writer is active or waiting.
     */
    void lock_read() {
        std::unique_lock<std::mutex> lock(mutex_);
        can_read.wait(lock, [this]() { return writing == false; });
        readers += 1;
    }
    /**
     * Release read lock.
     */
    void unlock_read() {
        std::unique_lock<std::mutex> lock(mutex_);
        readers -= 1;
        if (readers < 1) {
            can_write.notify_one();
        }
    }

    /**
     * Acquire write lock. Exclusive access.
     * Should block until all readers and writers are done.
     */
    void lock_write() {
        std::unique_lock<std::mutex> lock(mutex_);
        can_write.wait(lock, [this]() {
            return readers < 1 && !writing; // Check both conditions!
        });
        writing = true;
    }

    /**
     * Release write lock.
     */
    void unlock_write() {
        std::unique_lock<std::mutex> lock(mutex_);
        writing = false;
        can_read.notify_all();
        can_write.notify_all();
    }

    /**
     * RAII helpers
     */
    class ReadLockGuard {
    public:
        explicit ReadLockGuard(ReadWriteLock &rwlock) : rwlock_(rwlock) {
            rwlock_.lock_read();
        }
        ~ReadLockGuard() { rwlock_.unlock_read(); }

    private:
        ReadWriteLock &rwlock_;
    };

    class WriteLockGuard {
    public:
        explicit WriteLockGuard(ReadWriteLock &rwlock) : rwlock_(rwlock) {
            rwlock_.lock_write();
        }
        ~WriteLockGuard() { rwlock_.unlock_write(); }

    private:
        ReadWriteLock &rwlock_;
    };

private:
    std::condition_variable can_write;
    std::condition_variable can_read;
    mutable std::mutex mutex_;
    int readers{0};
    bool writing{false};
};

} // namespace concurrency