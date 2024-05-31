#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm> //find等方法
#include <memory> //智能指针
#include "algorithm/sort_namespace.h"
using namespace std; //可以使用标准库里的符号和方法


//一般的对象数组
class MyClass {
public:
  int value;

  MyClass() {
    value = 0;
  }

  void printValue() {
    std::cout << "Value: " << value << std::endl;
  }
};


int main(){
    
    //TODO: 输入和输出，目前暂缺输入
    cout << "hello world" << endl;


    //---String
    string str1;
    string str2 = "Hello World";
    string str3 = str2 + " How are you?";
    cout << str3 << endl;
    int len = str2.length();
    // 比较两个std::string对象
    if (str2 != str3) {
        cout << "str2 and str3 are not the same." << endl;
    }
    // 遍历std::string对象中的字符
    for (char c : str3) {
        cout << c << ' ';
    }
    //单纯换行
    cout << endl;
    // 获取和设置std::string对象中的特定字符
    char firstChar = str3[0];
    str3[0] = 'h';


    //----对象数组-基础，其他的容器放在vessel中讲
    const int arraySize = 5;
    
    //声明的时候**, 实例化的时候*
    //创建指向对象指针的指针数组
    //MyClass** myArray 表示 myArray 是一个指向指针的指针，
    //这些指针是指向MyClass对象的。
    //new MyClass*[arraySize] 则动态地创建了一个含有 arraySize 个元素的数组。
    //每一个元素都是一个可以指向MyClass的指针。
    MyClass** myArray = new MyClass*[arraySize];  

    // 创建对象并将指针存储在数组中
    for (int i = 0; i < arraySize; ++i) {
        myArray[i] = new MyClass();  // new在堆中分配内存，并返回地址
        myArray[i]->value = i + 1;   //-> 是通过地址访问实例吗？
    }

    // 使用对象数组的元素
    for (int i = 0; i < arraySize; ++i) {
        myArray[i]->printValue();  // 通过指针调用对象的成员函数
    }

    // 释放单个对象内存
    for (int i = 0; i < arraySize; ++i) {
        delete myArray[i];  // 使用 delete 运算符释放对象内存
    }
    // 释放数组内存
    delete[] myArray;  
    myArray = nullptr;


    return  0;



}