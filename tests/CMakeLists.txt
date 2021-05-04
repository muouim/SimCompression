##
# @file CMakeLists.txt
# @brief
# tests目录下cmake文件
#
# @author niexw
# @email xiaowen.nie.cn@gmail.com
#

project(utest)


# catch要求打开异常
string(REGEX REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

set(TEST test.cc sim/chunkerTest.cc)

add_library(test_src ${CMAKE_SOURCE_DIR}/src/chunker.cc ${CMAKE_SOURCE_DIR}/src/configure.cc ${CMAKE_SOURCE_DIR}/src/dector.cc ${CMAKE_SOURCE_DIR}/src/cryptoPrimitive.cc )

add_executable(utest ${TEST})
target_link_libraries(utest test_src)

find_package (Threads)
 
#add link library，添加可执行文件所需要的库，比如我们用到了libm.so（命名规则：lib+name+.so），就添加该库的名称
find_package (Threads)


link_directories(${PROJECT_SOURCE_DIR}/lib)


target_link_libraries(utest libcrypto.a )

target_link_libraries(utest libssl.a)

target_link_libraries(utest libboost_system.a )

target_link_libraries(utest libleveldb.a)


TARGET_LINK_LIBRARIES(utest dl)
TARGET_LINK_LIBRARIES(utest ${CMAKE_THREAD_LIBS_INIT})


