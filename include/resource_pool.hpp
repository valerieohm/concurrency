#pragma once
#include <semaphore>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <atomic>

namespace concurrency {

/**
 * Resource Pool using semaphores to limit concurrent access.
 * Demonstrates proper use of counting semaphores for resource management.
 * 
 * Real-world example: Database connection pool, thread pool, file handle pool
 */
template<typename Resource>
class ResourcePool {
public:
    explicit ResourcePool(size_t pool_size) 
        : available_resources_(pool_size)  // Semaphore initialized with pool size
        , total_acquisitions_(0)
        , total_releases_(0)
        , peak_usage_(0) {
        
        // Pre-populate the pool with resources
        for (size_t i = 0; i < pool_size; ++i) {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            resource_pool_.push(std::make_shared<Resource>(i));
        }
    }

    /**
     * Acquire a resource from the pool (blocking)
     * Uses semaphore to limit concurrent access
     */
    std::shared_ptr<Resource> acquire() {
        // Wait for available resource (semaphore decrements)
        available_resources_.acquire();
        
        std::shared_ptr<Resource> resource;
        {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            if (!resource_pool_.empty()) {
                resource = resource_pool_.front();
                resource_pool_.pop();
            }
        }
        
        // Update statistics
        total_acquisitions_.fetch_add(1);
        size_t current_usage = total_acquisitions_.load() - total_releases_.load();
        
        // Update peak usage atomically
        size_t current_peak = peak_usage_.load();
        while (current_usage > current_peak && 
               !peak_usage_.compare_exchange_weak(current_peak, current_usage)) {
            current_peak = peak_usage_.load();
        }
        
        return resource;
    }

    /**
     * Try to acquire a resource with timeout
     * Demonstrates timed semaphore operations
     */
    std::shared_ptr<Resource> try_acquire(std::chrono::milliseconds timeout) {
        // Try to acquire with timeout
        if (!available_resources_.try_acquire_for(timeout)) {
            return nullptr;  // Timeout - no resource available
        }
        
        std::shared_ptr<Resource> resource;
        {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            if (!resource_pool_.empty()) {
                resource = resource_pool_.front();
                resource_pool_.pop();
            }
        }
        
        total_acquisitions_.fetch_add(1);
        return resource;
    }

    /**
     * Release a resource back to the pool
     */
    void release(std::shared_ptr<Resource> resource) {
        if (!resource) return;
        
        {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            resource_pool_.push(resource);
        }
        
        total_releases_.fetch_add(1);
        
        // Signal that a resource is now available (semaphore increments)
        available_resources_.release();
    }

    // Statistics
    size_t available_count() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        return resource_pool_.size();
    }
    
    size_t total_acquisitions() const { return total_acquisitions_.load(); }
    size_t total_releases() const { return total_releases_.load(); }
    size_t peak_usage() const { return peak_usage_.load(); }
    size_t current_usage() const { 
        return total_acquisitions_.load() - total_releases_.load(); 
    }

private:
    std::counting_semaphore<> available_resources_;  // C++20 semaphore
    std::queue<std::shared_ptr<Resource>> resource_pool_;
    mutable std::mutex pool_mutex_;
    
    // Statistics
    std::atomic<size_t> total_acquisitions_;
    std::atomic<size_t> total_releases_;
    std::atomic<size_t> peak_usage_;
};

/**
 * RAII wrapper for automatic resource release
 */
template<typename Resource>
class ResourceGuard {
public:
    ResourceGuard(ResourcePool<Resource>& pool) 
        : pool_(pool), resource_(pool.acquire()) {}
    
    ResourceGuard(ResourcePool<Resource>& pool, std::chrono::milliseconds timeout) 
        : pool_(pool), resource_(pool.try_acquire(timeout)) {}
    
    ~ResourceGuard() {
        if (resource_) {
            pool_.release(resource_);
        }
    }
    
    // Non-copyable but movable
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
    
    ResourceGuard(ResourceGuard&& other) noexcept 
        : pool_(other.pool_), resource_(std::move(other.resource_)) {
        other.resource_ = nullptr;
    }
    
    Resource* operator->() const { return resource_.get(); }
    Resource& operator*() const { return *resource_; }
    
    bool valid() const { return resource_ != nullptr; }

private:
    ResourcePool<Resource>& pool_;
    std::shared_ptr<Resource> resource_;
};

} // namespace concurrency