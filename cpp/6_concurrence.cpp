#include <iostream>
#include <future>
using namespace std;

int asyncFunction() {
    return 43;
}

//async to syanc
void test_async(){

    future<int> result = async(launch::async,asyncFunction);
    int value = result.get();
    // 当然这样更好用
    // auto result = async(launch::async,asyncFunction);
    // auto value = result.get();
    cout << value << endl;
}


//互斥锁


//非互斥锁


//


//TODO，主要是并发和异步转同步的使用
int main(){
    test_async();
    return 0;
}