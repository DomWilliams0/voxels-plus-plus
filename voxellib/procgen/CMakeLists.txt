project(procgen)

set(SOURCES src/procgen.cpp src/procgen.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(${PROJECT_NAME} voxellib FastNoise)
