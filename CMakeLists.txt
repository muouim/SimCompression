#cmake version
cmake_minimum_required(VERSION 3.10)
project(simcomp)
#c++ version
set(CMAKE_CXX_STANDARD 14)


#head file path，头文件目录
INCLUDE_DIRECTORIES(include)
 
#source directory，源文件目录
AUX_SOURCE_DIRECTORY(src DIR_SRCS)

link_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(/usr/local/include/bsoncxx/v_noabi /usr/local/include/mongocxx/v_noabi)

#add executable file
add_executable(simcomp ${DIR_SRCS})
#add link library
IF (UNIX)
    find_package (Threads)
    link_libraries(sim)
    target_link_libraries(simcomp -lmongocxx)
    target_link_libraries(simcomp -lbsoncxx)

    target_link_libraries(simcomp libsim.a)
    target_link_libraries(simcomp libboost_system.a )
    target_link_libraries(simcomp libcrypto.a )
    target_link_libraries(simcomp libssl.a)
    target_link_libraries(simcomp libleveldb.a)

    TARGET_LINK_LIBRARIES(simcomp dl)
    TARGET_LINK_LIBRARIES(simcomp ${CMAKE_THREAD_LIBS_INIT})
ENDIF ()

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )

message(STATUS "### Done ###")
