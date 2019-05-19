cmake_minimum_required(VERSION 3.14)
project(procgen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -fno-stack-protector -pipe -O0")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(SOURCES
		src/procgen.cpp
		src/procgen.h
	)
include_directories(src)
include_directories(../src)
include_directories(../lib)
include_directories(../lib/glm/)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ../lib/glm ../lib)
