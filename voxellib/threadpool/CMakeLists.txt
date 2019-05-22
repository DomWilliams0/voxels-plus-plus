project(threadpool)

set(SOURCES threadpool.cpp threadpool.h)
set(SOURCES ${SOURCES} DKit/src/spmc/LinkedFIFO.h DKit/src/FIFO.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(Boost REQUIRED COMPONENTS system thread)
target_link_libraries(${PROJECT_NAME} Boost::thread)
target_include_directories(${PROJECT_NAME} PUBLIC DKit/src)

add_executable(threadpool_test main.cpp)
target_link_libraries(threadpool_test threadpool)