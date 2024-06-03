#include <iostream>
#include <future>
#include <vector>
#include <thread>
#include <mutex> //互斥
using namespace std;

//async to sync
void test_async(){
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
}


//互斥锁和条件变量
void test_mutex(){
    //TODO

}

//非互斥锁-原子操作
void test_automic(){
    //TODO
    
}

//读写锁
void test_read_write(){
    //TODO
}

//递归锁
void test_recursion(){
    //TODO
}

//自旋锁
void test_spin(){
    //TODO

}

//TODO，主要是并发和异步转同步的使用
int main(){
    test_async();
    return 0;
}