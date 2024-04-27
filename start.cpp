#include <iostream>
#include <future>
#include <vector>
#include <map>
#include <set>
#include <algorithm> //find等方法
#include <memory>
#include "algorithm/sortnamespace.h"
using namespace std; //可以使用标准库里的符号和方法
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

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

        //get和set
        string getName() const{
            return name;
        }
        void setName(string sname) {
            name = sname;
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

class B; //提前声明，方便在A中使用
class A {
    //若不刻意声明访问修饰符，则是private
    shared_ptr<B> b_ptr;
public:
    void setB(shared_ptr<B> b) { b_ptr = b; }
};
class B {
    weak_ptr<A> a_ptr; // 使用弱智能指针以避免循环引用
public:
    void setA(shared_ptr<A> a) { a_ptr = a; }
};


//cb，前边的bool是返回值，然后int是参数
//当然cb如果想更简单，可以用auto
void print_if(vector<int> const& vec, function<bool(int)> pred) {
    for(int i : vec) {
        if(pred(i)) {
            std::cout << i << '\n';
        }
    }
}


int main(){
    
    //TODO: 输入和输出，目前暂缺输入
    cout << "hello world" << endl;

    // //----对象数组
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

    //---容器--这个是自动释放的，动态数组
    // 创建一个空的 std::vector<Student> 容器
    //这个可以理解为Java的List
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

    // 删除某个，后边接index
    students.erase(students.begin() + 1);

    //find_if，对于动态数组
    //需要使用&，因为Student是一个引用类型
    //注意回调里得到的是引用或者值，如果是基本数据类型就是值，如果是对象就是引用
    auto compare = [](const Student& s)  { return s.getName() == "Alice"; };
    // 使用 find_if 进行查找

    auto its = std::find_if(students.begin(), students.end(), compare);
    // 上述返回的是指针，如果没有返回就等于end()
    if (its != students.end()) {
        std::cout << "Found Bob!" << std::endl;
    } else {
        std::cout << "Bob not found." << std::endl;
    }

    //循环展示，注意这里的&并不是取地址符，而是表示引用
    for(Student& student : students) {
        student.show();
    }

    auto modify = [](Student& s) 
    {
        if (s.getName() == "Alice") 
        {
            // 修改 Alice 的属性
            s.setName("Bob");
            // 打印信息
            std::cout << "Modified Alice's name to Bob!" << std::endl;
        }
    };
    //改-一般是在遍历的过程中改
    //for_each和transform
    for_each(students.begin(), students.end(), modify);

    //transform, 这个可以类比为JS中的map
    //注意回调里得到的是引用或者值，如果是基本数据类型就是值，如果是对象就是引用
    transform(students.begin(), students.end(), students.begin(), [](Student& student){ 
        student.age *= 2;
        return student;
    });



    //--如果是基础数据类型
    vector<int> v = {1, 2, 3, 4, 5};
    //--传参2: 基础数据类型的指针传递
    for (auto it = v.begin(); it != v.end(); ++it) {
        //这里为什么要加*
        cout << *it << endl;
    }

    vector<int> vvv = {1, 2, 3, 4, 5, 6};
    //--传参3:值传递，接受一个回调
    print_if(vvv, [](int i){ return i % 2 == 0; });
    

    auto it = find(v.begin(), v.end(), 3);
    if (it != v.end()) {
        std::cout <<  "Index" << *it << std::endl;
    } else {
        std::cout << "3 not found in vector v" << std::endl;
    }


    //总结：上述两个查的区别是：
    //简单来说，&在此被用于创建数组元素的引用，而*在此被用于通过迭代器访问元素的值。
    //使用引用是为了更改原对象，而*只是访问值

    //---async ，第一个参数是类方法，第二个参数是action function
    //其他基本上和别的
    future<int> result = async(launch::async,asyncFunction);
    int value = result.get();
    // 当然这样更好用
    // auto result = async(launch::async,asyncFunction);
    // auto value = result.get();
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

    MyStatic::SetCount(10);
    int count = MyStatic::GetCount();
    cout << count << endl;


    //指向对象的指针
    Derived* obj = new Derived();
    obj->show();

    //对象
    Derived obj1;
    obj1.show();

    //注意，以下这些14之后才能用
    //一些好用的c++特性, 在14之前，auto都不可以定义参数，但是14之后可以了
    auto lam= [] (auto x, auto y) -> auto { return x + y; };
    int r = lam(5, 6);  // result is 11



    //--smart ptr
    //资源获取即初始化，独占所有权，没有引用计数的开销，所以更轻量，用的较多
    auto ptr = make_unique<int>(5); 
    int c = *ptr;  //可以解引用，取值，但是不能把地址给到其他
    // unique_ptr拥有对象的独占所有权，一段地址唯一
    //所以我们不能通过以下方式创建一个unique_pt2指向这个整数，以下错误：
    // auto d = ptr;
    //作为参数和返回值


    //这里有引用计数的开销
    //1）make_shared<int>(5)创建了一个新的动态int对象，初始化为5。这时，创建的智能指针sp1是这个动态对象的唯一所有者，引用计数为1
    auto sp1 = make_shared<int>(5); 
    //2）然后，将sp1赋值给sp2。sp2现在也指向同一个动态对象，共享其所有权。所以，引用计数变为2。
    auto sp2 = sp1; 
    //3）读取sp1和sp2指向的对象的值并不会改变引用计数。
    int a = *sp1;    
    int b = *sp2;    
    // 4）最后，sp1和sp2都离开了它们的作用域，所以它们会被自动销毁。每销毁一个shared_ptr，引用计数减1。
    // 所以很多时候不用刻意考虑释放，出作用域就会自动帮你做的
    cout << b << endl;


    //weak_ptr，不过它没有自己的工厂方法，只是为了防止两个shared相互持有的循环引用
    //参见oc的weak strong dance
    auto aa = make_shared<A>();
    auto bb = make_shared<B>();
    aa->setB(bb);
    bb->setA(aa);


    //--map

    // 定义一个map对象，map也会自动释放
    map<int, string> mapStudent;
    
    // 第一种 用insert函數插入pair
    // 注意这里加的是pair，是一对儿的意思
    mapStudent.insert(pair<int, string>(000, "student_zero"));
    
    // 第二种 用insert函数插入value_type数据
    mapStudent.insert(map<int, string>::value_type(001, "student_one"));
    
    // 改
    mapStudent[000] = "student_first";
    mapStudent[456] = "student_second";

    //删除
    mapStudent.erase(001);

    //查 , 从key找，看这里find返回的也是迭代器指针
    if (mapStudent.find(000) != mapStudent.end()) {
        cout << "Found!" << endl;
    }

    //循环-解构循环
    for (const auto &[key, value] : mapStudent) {
        cout << key << ":" << value << endl;
    }

    //循环-pair循环
    for(const auto &pair : mapStudent){
        cout << pair.first  << ":" << pair.second << endl;
    }

    //改
    vector<string> values(mapStudent.size());
    //引用传递
    transform(mapStudent.begin(), mapStudent.end(), values.begin(), [](const std::pair<int, std::string>& pair) {
        return pair.second;
    }); 


    //TODO， --set
    set<int> mySet;
    // 添加（增）
    mySet.insert(1);
    mySet.insert(2);
    mySet.insert(5);
    mySet.insert(4);

    // 查找（查）
    auto search = mySet.find(2);
    if(search != mySet.end()) {
       std::cout << "元素 2 存在于集合中" << '\n';
    } else {
       std::cout << "元素 2 未找到" << '\n';
    }
    // 删除（删）
    mySet.erase(2);
    // 后续的查找再也找不到元素 2
    search = mySet.find(2);
    if(search != mySet.end()) {
        std::cout << "元素 2 存在于集合中" << '\n';
    } else {
        std::cout << "元素 2 未找到" << '\n';
    }


    //--namespace
    SortNamespace::quickSort();




    return  0;



}