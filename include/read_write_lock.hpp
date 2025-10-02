#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>

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
    
    ReadWriteLock(const ReadWriteLock&) = delete;
    ReadWriteLock& operator=(const ReadWriteLock&) = delete;
    
    /**
     * Acquire read lock. Multiple readers allowed simultaneously.
     * Should block if writer is active or waiting.
     */
    void lock_read();
    
    /**
     * Release read lock.
     */
    void unlock_read();
    
    /**
     * Acquire write lock. Exclusive access.
     * Should block until all readers and writers are done.
     */
    void lock_write();
    
    /**
     * Release write lock.
     */
    void unlock_write();
    
    /**
     * RAII helpers
     */
    class ReadLockGuard {
    public:
        explicit ReadLockGuard(ReadWriteLock& rwlock) : rwlock_(rwlock) {
            rwlock_.lock_read();
        }
        ~ReadLockGuard() { rwlock_.unlock_read(); }
    private:
        ReadWriteLock& rwlock_;
    };
    
    class WriteLockGuard {
    public:
        explicit WriteLockGuard(ReadWriteLock& rwlock) : rwlock_(rwlock) {
            rwlock_.lock_write();
        }
        ~WriteLockGuard() { rwlock_.unlock_write(); }
    private:
        ReadWriteLock& rwlock_;
    };

private:
    // TODO: Implement with proper state tracking and condition variables
};

} // namespace concurrency