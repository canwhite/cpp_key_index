#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm> //find等方法
using namespace std;


class Student {
public:
    string name;
    int age; 
    Student(){}
    Student(string n, int a) {
        name = n;
        age = a;
    }

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


};


//vector
static void test_vector(){

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
        student.age *= 2;
        return student;
    });

}


//map
static void test_map(){

    // 定义一个map对象，map也会自动释放
    map<int, string> mapString;
    
    //增
    mapString.insert(pair<int, string>(000, "student_zero"));
    mapString.insert(map<int, string>::value_type(001, "student_one"));
    

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
    

    //TODO:添加Sutdent


}



//set
static void test_set(){

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
    set<Student> setStudent;
    Student s1;
    s1.setName("张三");
    s1.setAge(20);
    setStudent.insert(s1);

    Student s2;
    s2.setName("李四");
    s2.setAge(22);
    setStudent.insert(s2);

    Student s3;
    s3.setName("王五");
    s3.setAge(19);
    setStudent.insert(s3);

    //如何查呢？
    




    // 使用范围for循环遍历std::set<Student>中的元素
    for (const auto& student : setStudent) {
        std::cout<< student.name << " "<< student.age<< std::endl;
    }


}



//TOOD：主要是数组、map和Set的使用
//这里每块儿的结构就是增删改查，如果缺失后续补充
int main(){

    test_vector();
    test_map();
    test_set();
    return  0;
}