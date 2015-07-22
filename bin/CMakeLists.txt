cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c11 -Wall -Wextra -pedantic -fno-stack-protector -fno-builtin -nostdinc -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -Wextra -pedantic -fno-stack-protector -fno-rtti -fno-exceptions -nostdinc -nostdlib")

add_subdirectory("test")
add_subdirectory("test_server")
