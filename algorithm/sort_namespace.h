#ifndef SORT_NAMESPACE_H
#define SORT_NAMESPACE_H
#include <vector>
using namespace std; //可以使用标准库里的符号和方法
namespace SortNamespace {
    //1) 快排
    void quickSort(vector<int> &arr , int left, int right);
    //2）选择
    void selectionSort(vector<int> &arr , int n);    
    //3) 冒泡
    void bubbleSort(vector<int> &arr, int n);

}   

#endif