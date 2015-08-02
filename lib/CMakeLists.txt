cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c11 -O2 -fpic -fno-stack-protector -fno-builtin -nostdinc -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic -std=c++11 -O2 -fpic -fno-stack-protector -fno-rtti -fno-exceptions -nostdinc -nostdlib")

add_subdirectory("libcrt")
add_subdirectory("libc")
