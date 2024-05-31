#include <iostream>
using namespace std;


class Base{
public:
    virtual void show(){
        cout << "In Base \n"; 
    };
};


//: 继承，访问修饰符
class Derived: public Base{
public:
    void show(){
        cout << "In Derived \n";
    };
};


void test_inheritance(){
    //指向对象的指针，然后指针访问
    Derived* obj = new Derived();
    obj->show();

    //对象，对象访问
    Derived obj1;
    obj1.show();
}

//todo
void test_polymorphism(){
    //TODO
}




int main(){
    test_inheritance();

    return 0;
}
