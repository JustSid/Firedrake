cmake_minimum_required(VERSION 2.8.9)
project(libcrt)

set(SOURCE start.c)

add_library(crt STATIC ${SOURCE})
