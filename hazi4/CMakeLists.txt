cmake_minimum_required(VERSION 3.10)
project(project)

# C++17 kell (std::regex, stb.)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# A futtatható neve "project", ahogy a feladat kéri: ./project --input ... --output ... --rgx 1
add_executable(project main.cpp)