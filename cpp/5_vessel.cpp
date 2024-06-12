#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm> //find等方法
#include <queue>
#include <list>
#include "../debug/video_debugging.h"
using namespace std;


class Student {
public:

    Student(){}
    //:后边是成员初始化列表，有了他们就不用在构造函数中初始化了
    Student(string n, int a) :name(n), age(a) {}
    // public方法主要是一些get和show
    void show() {
        cout << "Name: " << name << ", Age: " << age << endl;
    }

    string getName() const{
        return name;
    }

    void setName(string sname) {
        name = sname;
    } 

    //这里的const意味着getAge这个函数被声明为const，不能修改对象的状态
    int getAge() const{
        return age;
    }

    void setAge(int a){
        age = a;
    }

    // set添加的必须提供比较函数，以便std::set能够对元素进行排序和比较
    bool operator<(const Student& other) const {
        return name < other.name || (name == other.name && age< other.age);
    }
//私有属性只能被类的内部方法访问，而不能被类外部的任何方法访问。
//即使实例也不行，这是核心约束
private:
    string name;
    int age; 

};


//vector
//注意push_back和emplace_back可以insert闭包，这是一种很有意思的使用
//而且相比较来说，emplace_back 可以避免创建临时对象并进行拷贝或移动操作，
//所以，empalce_back的性能会更好
static void test_vector(){

    logging("============vector start===========");

    //1）---基础数据类型
    vector<int> v = {1, 2, 3, 4, 5};
    
    //循环
    for (auto it = v.begin(); it != v.end(); ++it) {
        //这里为什么要加*， *主要是为了访问值
        cout << *it << endl;
    }

    //find
    auto it = find(v.begin(), v.end(), 3);
    if (it != v.end()) {
        std::cout <<  "Index" << *it << std::endl;
    } else {
        std::cout << "3 not found in vector v" << std::endl;
    }




    //2）---动态数组
    // 创建一个空的 std::vector<Student> 容器
    //这个可以理解为Java的List
    vector<Student> students;

    //增
    students.push_back(Student("Alice", 18));
    students.push_back(Student("Bob", 19));
    students.push_back(Student("Charlie", 20));
    students.push_back(Student("David", 21));
    students.push_back(Student("Eve", 22));

    //删除
    students.pop_back();
    students.erase(students.begin() + 1);

    //改
    Student& alice = students[0];
    alice.setName("Linda");

    //查
    auto compare = [](const Student& s)  { return s.getName() == "Alice"; };

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

    // 长度
    cout << "The size of the vector is: " << students.size() << endl;

    //循环
    //for-in循环
    for (auto s : students) {
        s.show();
    }

    //for-each循环
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

    for_each(students.begin(), students.end(), modify);


    //transform循环，即map循环
    transform(students.begin(), students.end(), students.begin(), [](Student& student){ 
        // student.age *= 2;
        student.setAge(student.getAge()*2);
        return student;
    });


    logging("============vector end===========");

}


//map
static void test_map(){

    logging("============map start===========");

    // 定义一个map对象，map也会自动释放
    map<int, string> mapString;
    
    //增
    mapString.insert(pair<int, string>(000, "student_zero"));
    mapString.insert(map<int, string>::value_type(001, "student_one"));
    mapString.insert(make_pair(002, "student_two")); //推荐这一种，不需要专门定义类型
    

    //删
    mapString.erase(001);

    // 改
    mapString[000] = "student_first";
    mapString[456] = "student_second";


    //查 , 从key找，看这里find返回的也是迭代器指针
    if (mapString.find(000) != mapString.end()) {
        cout << "Found!" << endl;
    }

    //循环-解构循环
    for (const auto &[key, value] : mapString) {
        cout << key << ":" << value << endl;
    }

    //循环-pair循环
    for(const auto &pair : mapString){
        cout << pair.first  << ":" << pair.second << endl;
    }

    //循环-迭代器循环
    vector<string> values(mapString.size());
    //引用传递
    transform(mapString.begin(), mapString.end(), values.begin(), [](const std::pair<int, std::string>& pair) {
        return pair.second;
    }); 
    

    map<string,Student> studentsMap;
    //没有用new，是栈上的对象，这里向map中添加对象，但是没有new，所以这里不会释放
    //map会复制这些对象到其内存内部，这样的好处是避免了动态内存分配的开销，简化了内存管理
    studentsMap.insert(make_pair("001",Student("张三",20))); 
    studentsMap.insert(make_pair("002",Student("李四",12)));
    studentsMap.insert(make_pair("003",Student("王五",40)));

    //基本数据类型也能通过引用传递，但是没有必要
    for(const auto& pair : studentsMap){
        cout << pair.first  << ":" << pair.second.getName() << endl;
    }

    //find方法接受一个键作为参数，并返回一个迭代器，指向与该键关联的元素。
    //还是用key查的，所以用find就可以了
    auto it = studentsMap.find("001");
    //如果没找到，返回的迭代器等于end()
    if(it != studentsMap.end()) {
        // 在C++中，迭代器通常重载了->操作符，以便于访问和操作容器中的元素
        // 迭代器通常不支持点语法
        cout << it->second.getName() << endl;
    }

    logging("============map end===========");


}



//set
static void test_set(){

    logging("============set start===========");

    set<int> mySet;
    // 添加（增）
    mySet.insert(1);
    mySet.insert(2);
    mySet.insert(5);
    mySet.insert(4);

    // 删除（删），删除是直接删除值
    mySet.erase(2);

    // 改，由于set是自动排序的，所以无法修改
    // 由于std::set中的元素是唯一的，我们不能直接修改元素的值
    // 但是我们可以先删除旧元素，然后插入新元素
    int oldValue = 4;
    int newValue = 3;

    mySet.erase(oldValue);
    mySet.insert(newValue);

    // 查
    auto search = mySet.find(2);
    if(search != mySet.end()) {
       std::cout << "元素 2 存在于集合中" << '\n';
    } else {
       std::cout << "元素 2 未找到" << '\n';
    }

    // 后续的查找再也找不到元素 2
    search = mySet.find(2);
    if(search != mySet.end()) {
        std::cout << "元素 2 存在于集合中" << '\n';
    } else {
        std::cout << "元素 2 未找到" << '\n';
    }

    //循环
    for (const auto& elem : mySet) {
        cout<< elem << " ";
    }
    cout <<endl;

    //对象set，但是
    
    set<Student> studentsSet;
    
    //1）临时Student，利好编译器，没有额外的内存分配和复制操作
    studentsSet.insert(Student("张三",20));

    //2）相当于new了一个对象，会有额外的内存分配和复制操作
    Student s2;
    s2.setName("李四");
    s2.setAge(22);
    studentsSet.insert(s2);

    Student s3;
    s3.setName("王五");
    s3.setAge(19);
    studentsSet.insert(s3);

    //这样查的时候可以用find_if, 基础数据类型可以用find
    auto it = find_if(studentsSet.begin(),studentsSet.end(),[](const Student& student){
        return student.getName() == "李四";
    });

    if(it != studentsSet.end()) {
        cout << it->getName() << endl;
    }

    // 使用范围for循环遍历std::set<Student>中的元素
    for (const auto& student : studentsSet) {
        std::cout<< student.getName() << " "<< student.getAge()<< std::endl;
    }

    logging("============set end===========");


}


//queue
void static test_queue(){
    logging("============queue start===========");
    queue<Student> q;
    q.push(Student("张三",20));
    q.push(Student("李四",24));
    q.push(Student("王五",30));

    //删除头元素，注意pop不返回值
    q.pop();
    
    //修改头元素的信息
    q.front().setName("赵六");
    q.front().setAge(21);

    // 遍历队列并输出所有学生信息
    while (!q.empty()) {
        cout<< q.front().getName() << " "<< q.front().getAge()<< endl;
        q.pop();
    }

    logging("============queue end===========");
}

//list
//PS：注意以上容器中，只有这个是非线程安全的
//保持线程安全的主要做法是使用锁
void static test_list(){
    logging("============list start===========");
    //增
    list<Student> l;
    l.push_back(Student("张三",20));
    l.push_back(Student("李四",24));
    l.push_back(Student("王7",67));

    //删
    l.erase(l.begin());
    //改,for循环改
    //查，for循环查，我要这铁棒有何用
    //循环

    for (const auto& student : l) {
        std::cout<< student.getName() << " "<< student.getAge()<< std::endl;
    }
    logging("============list start===========");

}

//TOOD：主要是数组、map和Set的使用
//这里每块儿的结构就是增删改查，如果缺失后续补充
int main(){

    test_vector();
    test_map();
    test_set();
    test_queue();
    test_list();
    return  0;
}