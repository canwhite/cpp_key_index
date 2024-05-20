cmake_minimum_required(VERSION 3.10)

set(APP_NAME "APP")

project(${APP_NAME})

#vscode中除了cmake的配置也就是
#1)插件
#2)launch
#3)CMakeLists.txt
#关于c和c++版本，以及编译器相关在这里设置就可以了
#----- 
#指定c和c++的版本
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#指定c和c++的编译器
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")

#通过find_package管理三方lib
find_package(PkgConfig REQUIRED)

# 1. ffmpeg手动导入

# pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil)

# # 全局目标可用，如果在后边target_include_directories不行的时候可考虑这种
# include_directories(${FFMPEG_INCLUDE_DIRS})
# link_directories(${FFMPEG_LIBRARY_DIRS})

# 2. ffmpeg imported target导入，推荐
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libavfilter) 
# include_directories(${CMAKE_SOURCE_DIR}/algorithm)

#主文件
# set(SOURCE_FILES start.cpp)
# set(SOURCE_FILES ffmpeg/pre_parameter_passing.cpp)
# set(SOURCE_FILES ffmpeg/0_base.cpp)
# set(SOURCE_FILES ffmpeg/1_0_decoder.cpp)
# set(SOURCE_FILES ffmpeg/1_1_decoder.cpp)
# set(SOURCE_FILES ffmpeg/2_0_remuxing.cpp)
set(SOURCE_FILES  ffmpeg/4_0_add_watermarking.cpp)

# 引入algorithm和debug下的文件的文件
file(GLOB_RECURSE SORT_FILES  "algorithm/*.cpp" "algorithm/*.h" "debug/*.cpp" "debug/*.h")

# add var, 注意文件的相对位置
set(MY_VAR "Hello World")
set(IN_FILENAME ${CMAKE_SOURCE_DIR}/../test.mp4)
set(OUT_FILENAME ${CMAKE_SOURCE_DIR}/output.mp4)
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_SOURCE_DIR}/ffmpeg/config.h)

# 注意引入
add_executable(${APP_NAME} ${SOURCE_FILES} ${SORT_FILES})

# 1. ffmpeg手动导入
# target_link_libraries(${APP_NAME} ${FFMPEG_LIBRARIES})

# 2. impoted target
target_link_libraries(${APP_NAME} PkgConfig::FFMPEG)