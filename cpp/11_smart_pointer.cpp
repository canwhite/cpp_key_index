#include <iostream>
using namespace std;

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
//TOOD：智慧指针
int main(){
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
    return 0;
}