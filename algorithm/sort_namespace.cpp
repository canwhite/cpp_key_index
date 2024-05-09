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

    //2)选择排序，时间复杂度 n^2,将小元素交换到前边
    void selectionSort(vector<int> &arr , int n){
        int i, j, min_index;  //min_index用来标注最小值
        //时间复杂度是n^2
        for (int i = 0; i < n-1; i++)
        {
            //假设未排序的第一个元素是最小的数据
            min_index = i;
            //遍历未排序部分的其他数据 ，一一做对比
            for (int j = i+1; j < n; j++)
            {   
                if(arr[j] < arr[min_index])
                    min_index = j; //更新最小值的位置
            }
            swap(&arr[min_index],&arr[j]);  
        }
    }
    
    //3)冒泡，每次迭代，一一对比将大元素冒泡到后边
    void bubbleSort(vector<int> &arr ,int n){
        for (int i = 0; i < n-1; i++)
        {
            //i是外层循环，已经排序并冒泡到后边的个数，后续就不用再比对了
            for (int j = 0; j < n-1-i; j++)
            {
                //相邻元素比较
                if(arr[j] > arr[j+1]){
                    swap(&arr[j],&arr[j+1]);
                }
            }
        }
    }
    //4)插入排序-核心在于循环已排序部分，然后找到一个合适的插入位置
    void insertionSort(vector<int> &arr, int n){
        int i, key ,j;
        for (i = 1; i < n; i++)
        {
            //取出当前元素
            key = arr[i];
            //将当前索引前移一位，找到已排序的右边界
            //方便后续向左比对
            j = i - 1;
            //在i左侧寻找大于key的元素，并将其向右移
            //直到最左侧或者找到比key小的元素，它的后一位就是插入的位置
            while (j >=0  && arr[j] > key)
            {
                //这句是将较大的元素向右移一个位置，为插入 key 腾出空间。
                arr[j+1] = arr[j];
                //这句将扫描指针向左移一位，继续比对已排序序列中的元
                j--;
            }
            //最后在停止的位置后一位，插入key
            arr[j+1] = key;
        }
    }

    //5)归并排序
    void mergeSort(vector<int> &arr, int n){
        //TODO
        
    }
    



}