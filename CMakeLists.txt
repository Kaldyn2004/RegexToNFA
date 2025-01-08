cmake_minimum_required(VERSION 3.27)
project(RegexToNFA)
if(WIN32)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
set(CMAKE_CXX_STANDARD 20)

add_executable(RegexToNFA main.cpp)
