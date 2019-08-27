cmake_minimum_required(VERSION 3.0.0)
project(feverrpc VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(feverrpc main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include_directories("msgpack-3.2.0/include/")
include(CPack)
