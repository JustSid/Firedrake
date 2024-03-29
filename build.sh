#!/usr/bin/env bash

mkdir -p build
cd build

CMAKE_LINKER="<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
cmake "$(pwd)/.." -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="$(which clang)" -DCMAKE_C_COMPILER="$(which clang)" -DCMAKE_LINKER="$(which ld)" -DCMAKE_CXX_LINK_EXECUTABLE="${CMAKE_LINKER}" -DCMAKE_C_LINK_EXECUTABLE="${CMAKE_LINKER}"

make
make firedrake_iso
