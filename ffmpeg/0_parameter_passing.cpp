#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <algorithm> //find等方法
#include <memory>
using namespace std; //可以使用标准库里的符号和方法


//这是c++的结构体，区别于c的定义
struct Data {
    int a;
    int b;
};

/** 
typedef struct 
{
    int id;
    char* name; 
} Object;
*/

//值传递-C
void changeValue1(Data data) {
    data.a = 100;
    data.b = 100;
}

//指针传递-C
void changeValue2(Data* data) {
    data->a = 100;
    data->b = 100;
}

//引用传递-CPP
void changeValue(Data& data) {
    data.a = 100;
    data.b = 100;
}



int main (){

    Data data{1, 2};
    //C语言主要是两种，值传递和指针传递
    //1）值传递
    changeValue1(data);
    cout << data.a << " " << data.b;

    //2）指针传递，ffmpeg是c库，所以有些时候要用指针传递修改复杂值
    changeValue(&data);

    //C++多了个引用传递
    //3）引用传递
    changeValue3(data);


    return 0;
}
