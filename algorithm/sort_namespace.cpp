#include "sort_namespace.h" 

namespace SortNamespace {

    //swap交换，需要指针传递, 作为排序的通用方法
    void swap(int *a ,int *b ){
        int temp = *a;
        //这个转换两个地址的内容需要了解一下
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
            //当找到比pivot大的元素时，停止循环，
            //i<j , 左侧移动到j的前一个就可以了
            //arr[i] <= pivot, 但是左侧可以和pivot相等,这个也可以理解，排序吗
            while (i < j && arr[i] <= pivot) i++; 
            //当找到比pivot小的元素时，停止循环，
            //i <= j右侧可以移动到和左侧同一位置
            //arr[j] > pivot 右侧就需要比pivot大了
            while (i <= j && arr[j] > pivot) j--; 
            //若两者相遇，则停止
            if (i >= j) break; 
            //交换两个元素，使得左边的元素小于等于pivot，右边的元素大于pivot
            swap(&arr[i], &arr[j]); 
        }

        // 对左右子数组进行递归排序
        // 左边递归，
        // 这段代码的作用是对基准值左边的部分进行递归排序，也就是对数组中小于基准值的数进行排序。
        if(left < j) {
            quickSort(arr, left, j);
        }
        //右边递归
        if(i < right) {
            quickSort(arr, i, right);
        }
    }

    //2)选择
    void selectionSort(vector<int> &arr , int n){
        int i, j, min_index;  //min_index用来标注最小值
        //时间复杂度是n^2
        // for (int i = 0; i < n-1, i++)
        // {
            


        // }
        
        




    }
    
    //3)冒泡


}