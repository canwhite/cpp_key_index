#include<iostream>
#include<queue>
#include <mutex>
#include <thread>
#include <memory>
#include<condition_variable>


//线程安全的队列
template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        lock.unlock();
        cond_.notify_one();
    }

    bool try_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
};

int main() {
    ThreadSafeQueue<int> safe_queue;

    // 生产者线程
    std::thread producer([&safe_queue] {
        for (int i = 0; i < 10; ++i) {
            safe_queue.push(i);
            std::cout << "Produced: " << i << std::endl;
        }
    });

    // 消费者线程
    std::thread consumer([&safe_queue] {
        for (int i = 0; i < 10; ++i) {
            int value = safe_queue.pop();
            std::cout << "Consumed: "<< value<< std::endl;
        }
    });

    producer.join();
    consumer.join();

    return 0;
}