cmake_minimum_required(VERSION 3.15)
project(libcrt)

set(SOURCE start.c)

add_library(crt STATIC ${SOURCE})
