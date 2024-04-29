#include "sort_namespace.h" 

namespace SortNamespace {

    //需要指针传递
    void swap(int *a ,int *b ){
        int temp = *a;
        *a = *b;
        *b = temp;
    }

    //1）快排
    void quickSort(vector<int> &arr , int left, int right) {
        if(left >= right){
            return;
        }
        //选择中间元素作为基准
        int pivotIndex = left + (right-left)/2;
        int pivot = arr[pivotIndex];
        
        //分区操作
        //i，j是双指针，
        //i指向放置比pivot小的元素的位置，j指向放置比pivot大的元素的位置
        int i = left;
        int j = right;

        while (true) {
            //当找到比pivot大的元素时，停止循环，左侧移动到j的前一个就可以了
            while (i < j && arr[i] <= pivot) i++; 
            //当找到比pivot小的元素时，停止循环，右侧可以和左侧相等
            while (i <= j && arr[j] > pivot) j--; 
            //若两者相遇，则停止
            if (i >= j) break; 
            //交换两个元素，使得左边的元素小于等于pivot，右边的元素大于pivot
            swap(&arr[i], &arr[j]); 
        }

        // 对左右子数组进行递归排序
        // 左边递归
        if(left < j) {
            quickSort(arr, left, j);
        }
        //右边递归
        if(i < right) {
            quickSort(arr, i, right);
        }
    }
}