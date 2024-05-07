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
void changeValue3(Data& data) {
    data.a = 100;
    data.b = 200;
}


class Student {
public:
    std::string name;
    int age;
};


void processStudentByValue(Student student) {
    // Modify the student object here.
    student.name = "New Name";  // This won't affect the original Student.
}

void processStudentByPointer(Student* student) {
    // Modify the student object here.
    student->name = "New Name"; // This will affect the original Student.
}

void processStudentByReference(Student& student) {
    // Modify the student object here.
    student.name = "New Name";  // This will affect the original Student.
}

//指针的指针
void set_value(int **p, int new_value){
    //先*p 分配内存, 注意malloc默认返回的是void *，这里需要一个强转，只要用到malloc都需要这个强转
    *p = (int*)malloc(sizeof(int));
    //再**p分配值，这就是一个完整的过程了
    **p = new_value;
}


int main (){


    //一、---实际变量的情况
    Data data{1, 2};
    //C语言主要是两种，值传递和指针传递
    //1）值传递
    changeValue1(data);
    cout << data.a << " " << data.b <<endl;

    //2）指针传递，ffmpeg是c库，所以有些时候要用指针传递修改复杂值
    changeValue2(&data);
    cout << data.a << " " << data.b << endl;

    //C++多了个引用传递
    //3）引用传递
    changeValue3(data);
    cout << data.a << " " << data.b << endl;

    //二、---指针变量的情况
    //1) 值传递就要给一个实际的值，内部操作的也是值
    Student* student = new Student();
    processStudentByValue(*student);

    //2）指针传递-就需要传地址，内部用->操作
    processStudentByPointer(student);

    //3）引用传递，传入的是值，&表示对原值的引用，所以可以修改原值
    processStudentByReference(*student);


    //PS：指针传递的特例，指针的指针
    //大多数情况就像上边那样，但是还有一种情况是，
    //指针一开始没有分配内存，这就需要指针的指针来进行操作了，先分配内存，再分配值
    int *ptr = NULL;
    set_value(&ptr, 5);
    printf("%d\n", *ptr); // 这将打印5
    free(ptr);



    return 0;
}
