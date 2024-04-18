# cpp_key_index
c++ key index

## Doing:
1) 引入ffmpeg -done
2) 解码 - doing
3) 转码 - todo
4) 提取合并 - todo
5) 切割组合 - todo
6) 压缩 - todo
7) 流化 - todo


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


## PS:
[cpp-android-ios-example](https://github.com/canwhite/cpp-android-ios-example)