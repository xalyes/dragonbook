cmake_minimum_required(VERSION 3.10)

project(Compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME OFF)

if (WIN32)
    set (Boost_INCLUDE_DIR "C:\\boost_1_76_0")
    set (Boost_LIBRARY_DIR "C:\\boost_1_76_0\\stage\\lib")
endif ()

find_package(Boost COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} include)
link_directories(${Boost_LIBRARY_DIR})

add_executable(compiler main.cpp)

TARGET_LINK_LIBRARIES( compiler LINK_PUBLIC ${Boost_LIBRARIES} )
