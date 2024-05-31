#include <iostream>
#include <algorithm>
#include <vector>
#include "../algorithm/sort_namespace.h"

using namespace std;


void print_arr(vector<int> & arr){
    for (const auto &value : arr) {
        cout << value << ' ';
    }   
    cout << endl;
}


void test_sort(){
    //cpp本身就有一些计算方法
    //--sort and namespace

    //也可以用int arr[] = {} 创建，这样的arr是个指针
    vector<int> arr0 = {4,5,2,1,6,9};
    //1）快排
    SortNamespace::quickSort(arr0,0,arr0.size()-1);
    //输出结果
    print_arr(arr0);

    //2）选择排序
    vector<int> arr1 = {3,45,6,45,23,2};
    SortNamespace::selectionSort(arr1,arr1.size());
    print_arr(arr1);

    //3）冒泡排序    
    vector<int> arr2 = {4,5,2,1,45,66};
    //引用传值，实际上传递的是这个值本身，不需要传入地址
    SortNamespace::bubbleSort(arr2,arr2.size());
    print_arr(arr2);

    //4) 插入排序
    vector<int> arr3 = {7,2,3,445,34};
    SortNamespace::insertionSort(arr3,arr3.size());
    print_arr(arr3);



}


int main(){

    test_sort();

    return 0;
}