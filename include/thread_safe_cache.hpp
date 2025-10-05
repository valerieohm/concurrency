#ifndef THREAD_SAFE_CACHE
#define THREAD_SAFE_CACHE

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <mutex>
#include <unordered_map>

namespace concurrency {

/**
 * A thread-safe LRU cache with configurable capacity.
 * Should support concurrent reads and exclusive writes.
 */
template <typename Key, typename Value> class ThreadSafeCache {
public:
    explicit ThreadSafeCache(size_t capacity) : capacity_(capacity) {}
    ~ThreadSafeCache() = default; // hello

    ThreadSafeCache(const ThreadSafeCache &) = delete;
    ThreadSafeCache &operator=(const ThreadSafeCache &) = delete;
private:
    void pop_head() {
        if (!head_) {
            assert(tail_ == nullptr);
        } else {
            assert(head_->prev_ == nullptr);
            if (head_ == tail_) {
                assert(head_->next_ == nullptr);
                head_ = nullptr;
                tail_ = nullptr;
            } else {
                head_->next_->prev_ = nullptr;
                head_ = head_->next_;
            }
        }
    }
public:
    /**
     * Get value for key. Returns nullopt if not found.
     * Should allow concurrent reads.
     */
    std::optional<Value> get(const Key &key) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::optional<Value> result = std::nullopt;
        auto item = cache_.find(key);
        if (item != cache_.end()) {
            auto node = (*item).second;
            assert(node);
            result = node->value_;
            assert(head_);
            if (!tail_)
                assert(tail_);
            move_to_front(node);
            assert(head_ == node);
        }
        return result;
    }

    /**
     * Put key-value pair in cache.
     * Should evict LRU item if at capacity.
     * Requires exclusive access.
     */
    void put(const Key &key, const Value &value) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::shared_ptr<Node> node;
        if (cache_.contains(key)) {
            assert(head_);
            assert(tail_);
            auto item = cache_.find(key);
            assert(item != cache_.end());
            node = (*item).second;
            assert(node);
            node->value_ = value;
            move_to_front(node);
        } else {
            node = std::make_shared<Node>(key, value);
            cache_.insert(std::make_pair(key, node));
            assert(node->next_ == nullptr);
            node->next_ = head_;
            if (head_) head_->prev_ = node;
            else tail_ = node;
            head_ = node;
        }
        if (cache_.size() > capacity_) {
            assert(tail_);
            auto newtail = tail_->prev_;
            assert(newtail);
            newtail->next_ = nullptr;
            cache_.erase(tail_->key_);
            tail_ = newtail;
        }
    }
    

    /**
     * Remove key from cache if present.
     */
    bool remove(const Key &key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto item = cache_.find(key);
        if (item == cache_.end())
            return false;
        auto node = (*item).second;
        assert(node);
        auto prev = node->prev_;
        auto next = node->next_;
        node->prev_ = nullptr;
        node->next_ = nullptr;
        if (prev)
            prev->next_ = next;
        if (next)
            next->prev_ = prev;
        if (node == tail_)
            tail_ = prev;
        if (node == head_)
            head_ = next;
        assert(head_ ? head_->prev_ == nullptr : tail_ == true);
        cache_.erase(node->key_);
        return true;
    }

    /**
     * Get current size (may be stale immediately)
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);

        return cache_.size();
    }

    /**
     * Check if cache contains key (may be stale immediately)
     */
    bool contains(const Key &key) const {
        std::lock_guard<std::mutex> lock(mutex_);

        return cache_.contains(key);
    }

    /**
     * Clear all entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (head_) {
            auto next = head_->next_;
            head_->prev_ = nullptr;
            head_->next_ = nullptr;
            head_ = next;
        }
        tail_ = nullptr;
        return cache_.clear();
    }

private:
    mutable std::mutex mutex_;
    size_t capacity_;

    // LRU implementation: doubly linked list + hash map
    struct Node {
        Key key_;
        Value value_;
        std::shared_ptr<Node> prev_, next_;
        Node(Key const & key, Value const & value) :
        key_(key), value_(value), prev_(nullptr), next_(nullptr) {}
    };

    std::shared_ptr<Node> head_, tail_; // Dummy nodes
    std::unordered_map<Key, std::shared_ptr<Node>> cache_;

    void move_to_front(std::shared_ptr<Node> node) {
        if (node == head_)
            return;
        if (node == tail_)
            tail_ = node->prev_;
        auto prev = node->prev_;
        auto next = node->next_;
        if (prev)
            prev->next_ = next;
        if (next)
            next->prev_ = prev;
        node->prev_ = nullptr;
        node->next_ = head_;
        if (head_) head_->prev_ = node;
        head_ = node;
    }
};

} // namespace concurrency
#endif