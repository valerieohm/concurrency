#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <functional>

namespace concurrency {

/**
 * A thread-safe LRU cache with configurable capacity.
 * Should support concurrent reads and exclusive writes.
 */
template<typename Key, typename Value>
class ThreadSafeCache {
public:
    explicit ThreadSafeCache(size_t capacity);
    ~ThreadSafeCache() = default;
    
    ThreadSafeCache(const ThreadSafeCache&) = delete;
    ThreadSafeCache& operator=(const ThreadSafeCache&) = delete;
    
    /**
     * Get value for key. Returns nullopt if not found.
     * Should allow concurrent reads.
     */
    std::optional<Value> get(const Key& key);
    
    /**
     * Put key-value pair in cache.
     * Should evict LRU item if at capacity.
     * Requires exclusive access.
     */
    void put(const Key& key, const Value& value);
    
    /**
     * Remove key from cache if present.
     */
    bool remove(const Key& key);
    
    /**
     * Get current size (may be stale immediately)
     */
    size_t size() const;
    
    /**
     * Check if cache contains key (may be stale immediately)
     */
    bool contains(const Key& key) const;
    
    /**
     * Clear all entries
     */
    void clear();

private:
    size_t capacity_;
    // TODO: Implement with proper data structures and synchronization
};

} // namespace concurrency