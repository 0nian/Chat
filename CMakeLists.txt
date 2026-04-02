cmake_minimum_required(VERSION 3.0)
project(chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #使用C++17标准 不满足直接报错

#开启调试模式
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#头文件目录（仅扫描 .h 或 .hpp）
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/reactor)


#库文件路径（.a / .so / .lib）
#link_directories()

#手动指定要编译的文件
#set(SRC_LIST my.cpp)

#自动把当前目录所有 .cpp 文件加入编译
#aux_source_directory(. SRC_LIST)

#把代码编译成可执行文件 名字为server
#add_executable(server ${SRC_LIST})

#target_link_libraries(server muduo_net muduo_base pthread)
#设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#去src下找CMake继续编译
add_subdirectory(src)
