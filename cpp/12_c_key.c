#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct 
{
    int id;
    // char name[50]; //这样定义相当于提前预留50
    // 这里用char* 表示字符串
    char* name; 
} Object;


Object createObject(int id, char* name){
    Object obj;
    obj.id = id;

    //在C语言中，使用malloc或new等函数动态分配的内存空间，需要手动释放
    obj.name = malloc(50*sizeof(char));
    // strcpy(objects_1[i].name, "SomeString"+i);
    // strncpy有第三个参数size，是内存安全的
    strncpy(obj.name,name, 50);
    return obj;
}


int main(void){
    Object obj = createObject(1, "hello");
    printf("id: %d, name: %s\n", obj.id, obj.name);
    free(obj.name);

    //createObj是一个新的名字
    Object (*createObj)(int,char*) = &createObject;


    //malloc、calloc、realloc
    int n = 5;
    Object* objects_1 = (Object*)malloc(n * sizeof(Object));
    for (int i = 0; i < n; i++)
    {   
        //2）再初始化
        objects_1[i] = (*createObj)(i,"object_1_name");
        printf("%s \n",objects_1[i].name);
    }

    Object* objects_2 = (Object*)calloc(n,sizeof(Object));
    for (int i = 0; i < n; i++)
    {
        //2）再初始化
        objects_2[i] = (*createObj)(i,"object_2_name");
        printf("%s \n",objects_2[i].name);
    }



    /**
    calloc和malloc的最大区别
    malloc只分配区间，
    calloc在分配区间的同时，所有位都初始化为0
    */

    //realloc, -扩大objects_1的空间
    objects_1 = (Object*)realloc(objects_1, 10*sizeof(Object));
    for (int i = 5;  i < 10; i++)
    {
        //往这个空间上值
        objects_1[i] = (*createObj)(i,"object_1_name");
        printf("%s \n",objects_1[i].name);
    }



    //释放内存
    for (int i = 0; i < 10; i++)
    {
        free(objects_1[i].name);
    }
    for (int i = 0; i < n; i++)
    {
        free(objects_2[i].name);
        
    }
    free(objects_1);
    free(objects_2);
    

    return 0;
}