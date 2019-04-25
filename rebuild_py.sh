#!/usr/bin/env bash

mkdir build > /dev/null 2>&1
cd build
rm -rf library
cmake .. -DPYTHON=ON
cmake --build .