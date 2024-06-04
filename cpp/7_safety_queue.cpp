#include<iostream>
#include<queue>
#include <mutex>
#include<thread>
#include<memory>
#include<condition_variable>
using namespace std;

// 在 C++ 中，template<typename T> 是一个模板声明，用于定义泛型类或函数。
template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        unique_lock<mutex> lock(mutex_); 
        queue_.push(std::move(value)); 
        // lock.unlock(); // 释放互斥锁
        cond_.notify_one(); 
    }

    bool try_pop(T& value) {
        unique_lock<mutex> lock(mutex_); 
        if (queue_.empty()) { 
            return false; 
        }
        value = move(queue_.front());
        queue_.pop(); 
        return true; 
    }

    T pop() {
        unique_lock<mutex> lock(mutex_); 
        cond_.wait(lock, [this] { return !queue_.empty(); }); 
        T value = std::move(queue_.front()); 
        queue_.pop(); 
        return value; 
    }

    bool empty() const {
        lock_guard<mutex> lock(mutex_); 
        return queue_.empty(); 
    }

private:
    queue<T> queue_; // 队列
    mutable mutex mutex_; // 互斥锁
    condition_variable cond_; // 条件变量
};

int main() {
    ThreadSafeQueue<int> safe_queue; // 创建线程安全的队列

    // 生产者线程
    // 不接受参数的时候()可以省略
    thread producer([&safe_queue]() {
        for (int i = 0; i < 10; ++i) {
            safe_queue.push(i); // 向队列中添加元素
            cout << "Produced: " << i << endl; // 输出生产的元素
        }
    });

    // 消费者线程
    thread consumer([&safe_queue]() {
        for (int i = 0; i < 10; ++i) {
            int value = safe_queue.pop(); // 从队列中取出元素
            cout << "Consumed: "<< value<< endl; // 输出消费的元素
        }
    });

    producer.join(); // 等待生产者线程结束
    consumer.join(); // 等待消费者线程结束

    return 0;
}