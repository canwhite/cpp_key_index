#include <iostream>
using namespace std;


//直接实现静态方法
static void logging(const char *fmt, ...);
// implement
static void logging(const char *fmt, ...){
    //首先声明了一个名为args的变量，类型为va_list。这是处理变长参数列表时使用的一种类型。
    va_list args; 
    //这行代码向标准错误(stderr)输出"LOG: "。
    fprintf( stderr, "LOG: " );
    //这行代码的作用是获取变长参数列表。函数参数fmt是一个指向字符常量的指针，它指定了传递给函数的变长参数的数量和类型。
    va_start( args, fmt );
    //这行代码将可变参数列表args按照fmt指定的格式输出到标准错误(stderr)。
    vfprintf( stderr, fmt, args );
    //这行代码的作用是结束对变长参数列表的访问，释放资源。
    va_end( args ); 
    //这行代码在错误信息后打印一个换行符，使得各个日志之间保持清晰和整齐。
    fprintf( stderr, "\n" ); 
}



//静态类
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

private:
    static int m_count;
}; //注意类定义完了要加分号


// 注意static属性是堆里，默认已经初始化，
// 不能加在方法里初始化，因为方法里是到那个位置才初始化
int MyStatic::m_count = 0;  // 静态成员的初始化



int main(){

    MyStatic::SetCount(10);
    int count = MyStatic::GetCount();
    cout << count << endl;

    logging("123");

    return 0;

}