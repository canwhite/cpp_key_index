#include<iostream>
#include<vector>
#include<queue>
#include<thread>
#include <mutex>
#include<condition_variable>
#include<functional>
using namespace std;

class ThreadPool {
public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return !tasks.empty() || stop; });
                        if (stop) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    //在这个代码片段中，F&& f 和 Args&&... args 都使用了右值引用和通用引用（也称为转发引用）。
    //这是为了实现完美转发，即将参数以原始类型（左值或右值）传递给另一个函数，而不改变参数的类型。
    //这样可以保持参数的原始类型，并实现移动语义和避免不必要的拷贝。
    void enqueue(F&& f, Args&&... args) {
        {
            // 创建一个 unique_lock 对象，用于锁定互斥量 queue_mutex
            // 这里使用了 std::unique_lock 而不是 std::lock_guard，因为我们需要在锁定互斥量后执行一些操作，然后再解锁
            unique_lock<mutex> lock(queue_mutex);

            // 使用 std::bind 将可调用对象 f 和参数 args 绑定到一个新的可调用对象中
            // 这里使用了 std::forward 来保持参数的原始类型（左值或右值）int a = 10+20; a就是左值，10+20就是右值
            // 然后使用 emplace 方法将新的可调用对象添加到任务队列 tasks 中
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        // 在锁定互斥量的作用域结束后，通知一个等待的线程（如果有的话）
        // 这样，等待的线程可以检查任务队列并执行新添加的任务
        condition.notify_one();
    }

private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop = false;
};

int main() {
    ThreadPool pool(4);

    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i] {
            cout << "Task " << i << " is running on thread "<< this_thread::get_id()<< endl;
            this_thread::sleep_for(chrono::seconds(1));
        });
    }

    return 0;
}