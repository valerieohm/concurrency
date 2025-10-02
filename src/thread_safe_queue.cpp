#include <thread_safe_queue.hpp>

using namespace concurrency;

template<typename T>
void ThreadSafeQueue<T>::push(const T& item) {
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.emplace_back(item);
}

template<typename T>
void ThreadSafeQueue<T>::push(T&& item) {
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.emplace_back(item);
}
template <typename T> std::optional<T> ThreadSafeQueue<T>::try_pop() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::optional<T> popped;
    if (!mQueue.empty()) {
        popped = mQueue.front();
        mQueue.pop();
    }
    return popped;
}

template<typename T>
T ThreadSafeQueue<T>::wait_and_pop(){}

template<typename T>
bool ThreadSafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.empty();
}

template<typename T>
size_t ThreadSafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.size();
}
template<typename T>
void ThreadSafeQueue<T>::shutdown(){}



