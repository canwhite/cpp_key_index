#include<iostream>
#include<queue>
#include <mutex>
#include<thread>
#include<memory>
#include<condition_variable>
using namespace std;

// 线程安全的队列
template<typename T>
class ThreadSafeQueue {
public:
    // 向队列中添加元素
    void push(T value) {
        unique_lock<mutex> lock(mutex_); // 获取互斥锁
        queue_.push(move(value)); // 将元素添加到队列中
        lock.unlock(); // 释放互斥锁
        cond_.notify_one(); // 通知等待的线程
    }

    // 尝试从队列中取出元素，如果队列为空则返回false
    bool try_pop(T& value) {
        unique_lock<mutex> lock(mutex_); // 获取互斥锁
        if (queue_.empty()) { // 如果队列为空
            return false; // 返回false
        }
        value = move(queue_.front()); // 取出队列中的第一个元素
        queue_.pop(); // 删除队列中的第一个元素
        return true; // 返回true
    }

    // 从队列中取出元素，如果队列为空则等待
    T pop() {
        unique_lock<mutex> lock(mutex_); // 获取互斥锁
        cond_.wait(lock, [this] { return !queue_.empty(); }); // 等待队列非空
        T value = move(queue_.front()); // 取出队列中的第一个元素
        queue_.pop(); // 删除队列中的第一个元素
        return value; // 返回元素
    }

    // 判断队列是否为空
    bool empty() const {
        lock_guard<mutex> lock(mutex_); // 获取互斥锁
        return queue_.empty(); // 返回队列是否为空
    }

private:
    queue<T> queue_; // 队列
    mutable mutex mutex_; // 互斥锁
    condition_variable cond_; // 条件变量
};

int main() {
    ThreadSafeQueue<int> safe_queue; // 创建线程安全的队列

    // 生产者线程
    thread producer([&safe_queue] {
        for (int i = 0; i < 10; ++i) {
            safe_queue.push(i); // 向队列中添加元素
            cout << "Produced: " << i << endl; // 输出生产的元素
        }
    });

    // 消费者线程
    thread consumer([&safe_queue] {
        for (int i = 0; i < 10; ++i) {
            int value = safe_queue.pop(); // 从队列中取出元素
            cout << "Consumed: "<< value<< endl; // 输出消费的元素
        }
    });

    producer.join(); // 等待生产者线程结束
    consumer.join(); // 等待消费者线程结束

    return 0;
}