#include <iostream>
#include <vector>
#include <functional>
using namespace std;


void test_base(){   
    //注意，以下这些14之后才能用，在14之前，auto都不可以定义参数，但是14之后可以了
    //还有就是这里的箭头函数，-> returnType block
    auto lam= [] (auto x, auto y) -> auto { return x + y; };
    int r = lam(5, 6);  // result is 11

}

void test_reference(){
    //引用外部值并修改
    int x = 10;
    //mutable是对整个闭包的形容
    //c++ 23 的时候可以省略（）
    //这里也可以只放一个&,作为默认捕获，以引用的方式捕获当前作用域中的所有变量
    auto lambda = [&x] () mutable { 
        //这里能换行主要是因为endl
        cout << "x = " << x << endl; 
        x += 10; 
    }; // lambda captures x by reference

    lambda();
    cout << "After lambda, x = " << x << endl;

}

//callback部分，外层function<>类型声明，
//然后<>中，bool(int) ，前边的bool是返回值，然后int是参数
//当然callback如果想更简单，可以用auto
void print_if(vector<int> const& vec, function<bool(int)> pred) {
    for(int i : vec) {
        if(pred(i)) {
            std::cout << i << '\n';
        }
    }
}


void test_callback(){
    vector<int> vvv = {1, 2, 3, 4, 5, 6};
    print_if(vvv, [](int i){ return i % 2 == 0; });
}



//闭包相关
int main(){
    test_base();
    test_reference();
    test_callback();

    return 0;
}