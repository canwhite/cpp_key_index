#include <iostream>
#include <future>
#include <vector>
using namespace std; //可以使用标准库里的符号和方法


int asyncFunction() {
    return 43;
}

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
//Vector的对象数组
class Student {
    public:
        string name; // 姓名
        int age; // 年龄
        // 构造函数，用于初始化对象的成员变量
        Student(string n, int a) {
            name = n;
            age = a;
        }
        // 成员函数，用于显示对象的信息
        void show() {
            cout << "Name: " << name << ", Age: " << age << endl;
        }
};

class MyStatic
{
//public一般定义一些get和set方法,用于modal的取用
public:
    static int GetCount(){
        return m_count;
    }
    static void SetCount(int count){
        m_count = count;
    }
//private一般定义一些
private:
    static int m_count;
}; //注意类定义完了要加分号

// 注意static属性是类，所以不能加在方法里，因为方法里是到那个位置才初始化
int MyStatic::m_count = 0;  // 静态成员的初始化
    
int main(){
    
    //TODO: 输入和输出，目前暂缺输入
    cout << "hello world" << endl;

    // //----对象数组
    const int arraySize = 5;

    MyClass** myArray = new MyClass*[arraySize];  // 创建指向对象指针的指针数组

    // 创建对象并将指针存储在数组中
    for (int i = 0; i < arraySize; ++i) {
        myArray[i] = new MyClass();  // 使用 new 运算符为每个指针分配内存
        myArray[i]->value = i + 1;   // 通过指针访问对象的成员变量
    }

    // 使用对象数组的元素
    for (int i = 0; i < arraySize; ++i) {
        myArray[i]->printValue();  // 通过指针调用对象的成员函数
    }

    // 释放对象内存
    for (int i = 0; i < arraySize; ++i) {
        delete myArray[i];  // 使用 delete 运算符释放对象内存
    }
    delete[] myArray;  // 释放指针数组的内存
    myArray = nullptr;


    //---容器--这个是自动释放的
    // 创建一个空的 std::vector<Student> 容器
    vector<Student> students;

    // 向容器中添加 5 个 Student 对象
    students.push_back(Student("Alice", 18));
    students.push_back(Student("Bob", 19));
    students.push_back(Student("Charlie", 20));
    students.push_back(Student("David", 21));
    students.push_back(Student("Eve", 22));

    // 显示容器中的元素个数
    cout << "The size of the vector is: " << students.size() << endl;

    // 使用下标访问容器中的第一个元素，并调用其成员函数
    students[0].show();

    // 使用迭代器访问容器中的最后一个元素，并修改其成员变量
    students.back().name = "Eva";

    // 使用 for 循环遍历容器中的所有元素，并显示它们的信息
    for (auto s : students) {
        s.show();
    }

    // 从容器中删除第二个元素，并将后面的元素向前移动
    students.erase(students.begin() + 1);

    // 显示容器中的元素个数
    cout << "The size of the vector is: " << students.size() << endl;

    for (auto s : students) {
        s.show();
    }


    //---async
    future<int> result = async(launch::async,asyncFunction);
    int value = result.get();
    cout << value << endl;

    //---callback
    int x = 10;
    //因为这里引用捕获了，所以内部可以修改x的值
    auto lambda = [&x]() mutable { 
        //这里能换行主要是因为endl
        cout << "x = " << x << endl; 
        x += 10; 
    }; // lambda captures x by reference

    lambda();
    cout << "After lambda, x = " << x << endl;

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



    //---Static Function
    //TODO, Static，
    MyStatic::SetCount(10);
    int count = MyStatic::GetCount();
    cout << count << endl;



    return  0;



}