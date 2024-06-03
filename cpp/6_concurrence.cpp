#include <iostream>
#include <future>
#include <vector>
#include <thread>
#include <mutex> //互斥
#include <shared_mutex>
#include <atomic>
#include<queue>
#include "../debug/video_debugging.h"
using namespace std;

//async to sync
void test_async(){

    //1）------使用async
    logging("============async to sync start===========");
    //PS：第一个参数是类launch::async方法
    //返回值是future
    future<int> result = async(launch::async,[](){
        return 43;
    });
    //当我们调用 fut.get() 时，主线程会阻塞，直到 async_function 的返回值可用。
    int value = result.get();
    // 当然这样更好用
    // auto result = async(launch::async,asyncFunction);
    // auto value = result.get();
    cout << value << endl;

    //2）----获取线程返回值，这个最终结果和上述一致
    





    logging("============async to sync end===========");
}


//1）-------互斥锁和条件变量
//FE: 基本锁类型，保护共享资源免受多个线程并发访问。条件变量用于线程间的通信

mutex mtx;
condition_variable cv;
queue<int> data_queue;

//生产者消费者模式，是一种常见的多线程编程方式
void test_mutex(){
    logging("============mutex start===========");  
    //生产者线程
    thread producer_thread([](){
        //生产数据
        for(int i = 0; i < 10; ++i){
            //注意unique_lock在构造时会自动锁定互斥锁，
            //并在析构时自动解锁互斥锁。
            unique_lock<mutex> lock(mtx);
            //添加数据,push在末尾加
            data_queue.push(i);
            // lock.unlock();
            //cv通知消费者线程
            cv.notify_one();
        }
    });

    //消费者线程
    thread consumer_thread([](){
        while (true)
        {
            unique_lock<mutex> lock(mtx);
            // cv等待有数据可消费
            cv.wait(lock, [](){return !data_queue.empty();});
            //front从queue前端取，但是不删除，back从后端取但是不删除
            int data = data_queue.front();
            //从前端移除
            data_queue.pop();
            std::cout << "Consumed: "<< data<< std::endl;
            if (data == 9) break; // 如果消费完所有数据，退出循环
        } 
    });

    producer_thread.join();
    consumer_thread.join();

    logging("============mutex end===========");

}

//2）------非互斥锁-原子操作
//FE：原子操作是一种不可中断的操作，可以确保在多线程环境中不会发生竞争条件
//QE: 为什么原子操作不可中断呢
//AN: 是基于硬件锁或者内存协议来实现的

atomic<int> counter(0); //一般用于技术信号量，通过load拿到值，如果<=0不执行之类的

void test_atomic(){
    logging("============atomic start===========");  
    //初始化线程个数
    const int num_threads = 4;
    thread threads[num_threads];
    //循环，单个线程里边进行单个事件    
    for(int i = 0; i < num_threads; i++){
        //&是一个默认捕获，以引用的方式捕获当前作用域中的所有变量
        threads[i] = thread([&](){
            for(int i = 0; i < 100; i++){
                counter.fetch_add(1);
            }
        });
    }

    //批量等待
    for (auto& t : threads) {
        t.join();
    }

    cout << "Counter value: "<< counter<< endl;
    logging("============atomic end===========");  
}


//3）------------读写锁
//FE：读写锁允许多个线程同时读取共享资源，但在写入时只允许一个线程访问。
//这是一个读写锁（read-write lock），它允许多个线程同时读取共享数据，但在写入数据时会独占访问。
shared_mutex rw_lock; // 定义一个读写锁
string rw_data = "init"; // 定义一个共享数据字符串
bool ready = false; // 定义一个标志位，用于同步线程的启动
//--当然也使用了前边的条件变量和mutex

void test_read_write(){
    logging("============read write start===========");  
    vector<thread> threads; // 定义一个线程向量

    const auto writer = [&](const string& new_data){
        //独占
        std::unique_lock<mutex> lck(mtx);
        while (!ready) {
            cv.wait(lck);
        }
        rw_data = new_data;
        cout << "数据已更新: "<< rw_data<< endl;
    };

    const auto reader = [&]() {
        //共享
        shared_lock<shared_mutex> lck(rw_lock);
        cout << "读取到的数据: "<< rw_data<< endl; // 输出读取到的数据
        // return rw_data; // thread并不支持返回值，可以使用promise和feature
    };


    //--写,使用unique_lock独占,PS：注意这里可以直接加回调作为线程
    threads.emplace_back(writer, "Hello, World!"); // 在线程向量末尾就地构造一个写线程

    //--读，shared_lock它允许多个线程同时读取共享数据
    for(int i = 0; i< 3; i++){
        threads.emplace_back(reader);
    }

    //--trigger
    //因为内部使用了unique_lock，为了方便释放，所以使用逻辑块儿
    {
        //获取互斥锁
        unique_lock<mutex> lck(mtx);
        //设置标志位
        ready = true;
        //默认释放互斥锁
    }
    //通知所有等待线程，通知都是用cv
    cv.notify_all();

    //--再模拟外部读操作
    for(int i = 0; i< 3; i++){
        threads.emplace_back(reader);
    }

    //批量等待线程结束
    for (auto& t : threads) {
        t.join();
    }

    logging("============read write end==========="); 
}

//4）-------------递归锁
//FE ：递归锁允许同一线程多次获得锁，而不会导致死锁。利好递归操作
void test_recursion(){
    //TODO
    logging("============recursion  start===========");  

    logging("============recursion  end===========");  
}

//5）-------------自旋锁
//FE：自旋锁不会让线程休眠，而是忙等待，一直检查，直到锁可用，适用于线程切换开销太大的场景
//QE：哪些情况下切换线程开销大呢？
//AN: 锁持有时间非常短，高优先级线程，实时系统，避免死锁（自旋锁会竞争过互斥锁）
void test_spin(){
    //TODO
    logging("============spin  start===========");  

    logging("============spin  end===========");  

}

//TODO，主要是并发和异步转同步的使用
int main(){
    test_async();
    test_mutex();
    test_atomic();
    test_read_write();
    return 0;
}