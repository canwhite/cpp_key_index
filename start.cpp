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
    
class Base{
public:
    virtual void show(){
        cout << "In Base \n"; 
    };
};

//继承实现
//: 继承，访问修饰符
class Derived: public Base{
public:
    void show(){
        cout << "In Derived \n";
    };
};


int main(){
    
    //TODO: 输入和输出，目前暂缺输入
    cout << "hello world" << endl;

    // //----对象数组
    const int arraySize = 5;
    
    //声明的时候**, 实例化的时候*
    //创建指向对象指针的指针数组
    MyClass** myArray = new MyClass*[arraySize];  

    // 创建对象并将指针存储在数组中
    for (int i = 0; i < arraySize; ++i) {
        myArray[i] = new MyClass();  // 使用 new 运算符为每个指针分配内存
        myArray[i]->value = i + 1;   // 通过指针访问对象的成员变量
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


    //---容器--这个是自动释放的
    // 创建一个空的 std::vector<Student> 容器
    vector<Student> students;

    //push 和 pop
    students.push_back(Student("Alice", 18));
    students.push_back(Student("Bob", 19));
    students.push_back(Student("Charlie", 20));
    students.push_back(Student("David", 21));
    students.push_back(Student("Eve", 22));

    //pop，移除最后一个实例
    students.pop_back();

    // 长度
    cout << "The size of the vector is: " << students.size() << endl;

    //循环展示，auto
    for (auto s : students) {
        s.show();
    }

    cout << "---erase---" << endl;

    // 删除某个,发音， ɪˈreɪs 抹去，后边接index
    students.erase(students.begin() + 1);

    //循环展示，注意这里的&并不是取地址符，而是表示引用
    for(Student& student : students) {
        student.show();
    }


    //---async ，第一个参数是类方法，第二个参数是action function
    //其他基本上和别的
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


    //指向对象的指针
    Derived* obj = new Derived();
    obj->show();

    //对象
    Derived obj1;
    obj1.show();


    return  0;



}