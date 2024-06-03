#include <iostream>
#include <future>
#include <vector>
#include <thread>
#include <mutex> //互斥
#include <atomic>
#include<queue>
#include "../debug/video_debugging.h"
using namespace std;

//async to sync
void test_async(){
    logging("============async to sync start===========");
    //PS：第一个参数是类launch::async方法
    //返回值是future
    future<int> result = async(launch::async,[](){
        return 43;
    });
    int value = result.get();
    // 当然这样更好用
    // auto result = async(launch::async,asyncFunction);
    // auto value = result.get();
    cout << value << endl;
    logging("============async to sync end===========");
}


//互斥锁和条件变量
//FE: 基本锁类型，保护共享资源免受多个线程并发访问。条件变量用于线程间的通信

mutex mtx;
condition_variable cv;
queue<int> data_queue;

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

//非互斥锁-原子操作
//FE：原子操作是一种不可中断的操作，可以确保在多线程环境中不会发生竞争条件
void test_atomic(){
    logging("============atomic start===========");  

    logging("============atomic end===========");  
    
}

//读写锁
//FE：读写锁允许多个线程同时读取共享资源，但在写入时只允许一个线程访问。
void test_read_write(){
    //TODO
    logging("============read write start===========");  

    logging("============read write end==========="); 
}

//递归锁
//FE ：递归锁允许同一线程多次获得锁，而不会导致死锁。利好递归操作
void test_recursion(){
    //TODO
    logging("============recursion  start===========");  

    logging("============recursion  end===========");  
}

//自旋锁
//FE：自旋锁不会让线程休眠，而是忙等待，一直检查，直到锁可用，适用于线程切换开销太大的场景
void test_spin(){
    //TODO
    logging("============spin  start===========");  

    logging("============spin  end===========");  

}

//TODO，主要是并发和异步转同步的使用
int main(){
    test_async();
    test_mutex();
    return 0;
}