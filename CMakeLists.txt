cmake_minimum_required(VERSION 3.0)
project(chat)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

#加载子目录

#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
#set(SRC_LIST my.cpp)
#aux_source_directory(. SRC_LIST)

#add_executable(server ${SRC_LIST})

#target_link_libraries(server muduo_net muduo_base pthread)
#配置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(src)
