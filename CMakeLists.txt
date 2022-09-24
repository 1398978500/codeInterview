project(CodeInterview)

cmake_minimum_required(VERSION 3.10)

add_compile_options(-std=c++11 -g)
# set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

set(REDIS_PLUS_PLUS_DEFAULT_CXX_STANDARD 11)

add_subdirectory(ThirdPirty)
add_subdirectory(Test)
