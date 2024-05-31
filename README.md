# cpp_key_index
pre:     
c++ key index    
cpp/0_start.cpp and other features - done  

## Doing:
/algorithm 
1) 排序 - doing
2) 聚合 - todo
3) 回溯 - todo
4) 树   - todo
5) 动规 - todo
6) 单调栈 - todo
7) ...

/ffmpeg 
1) 引入ffmpeg -done
2) 解码 - done
3) 重编码 - done
4) 转码 - done
4) 加减水印 - doing
6) 推流 - todo
7) 提取合并 - todo
8) 压缩 - todo

PS: cmake设置和读取变量   
```
pre. --根目录下创建config.h.in
写入变量定义
#define IN_FLIENAME "${IN_FILENAME}"

a.--CMakeLists.txt中设置变量
set(IN_FILENAME ${CMAKE_SOURCE_DIR}/../test.mp4)

b.--CMakeLists.txt命令行根据 config.h.in创建 config.h 
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_SOURCE_DIR}/ffmpeg/config.h)

c.--使用的地方引入config.h 
...
#include "config.h"
...
cout << IN_FLIENAME << endl;

````

## TODO：
创建一个RN的项目     
方案一：

1. 单独提出一个sdk文件夹在RN项目
里边使用C++调用FFmpeg等类库
2. 在原生项目中植入我们的C++ sdk
3. 原生给RN提供调用方法
4. 在RN中实现音频类需求


方案二：    
1. 用EMScripten将C++的调用转为ES6
2. 然后在RN中直接调用使用


然后再引入webrtc


## PS:
[cpp-android-ios-example](https://github.com/canwhite/cpp-android-ios-example)