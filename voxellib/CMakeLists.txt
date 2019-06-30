project(voxellib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")


set(SOURCES src/game.cpp src/game.h src/world/world.cpp src/world/world.h src/error.h src/world/world_renderer.cpp src/world/world_renderer.h src/shader_loader.cpp src/shader_loader.h src/util.cpp src/util.h src/camera.cpp src/camera.h src/world/chunk.cpp src/world/chunk.h src/world/block.h src/world/face.h src/world/face.cpp src/world/centre.h src/ui.cpp src/ui.h src/world/generation/generator.cpp src/world/generation/generator.h src/world/loader.cpp src/world/loader.h src/game_entry.cpp src/game_entry.h src/config.cpp src/config.h src/constants.h src/constants.h src/world/iterators.h src/world/chunk_load/state.cpp src/world/chunk_load/state.h src/world/chunk_load/double_buffered.h src/world/terrain.cpp src/world/terrain.h src/multidim.h src/world/block.cpp)

# imgui
add_subdirectory(lib/imgui EXCLUDE_FROM_ALL)

# fastnoise
add_library(FastNoise STATIC lib/FastNoise/FastNoise.h lib/FastNoise/FastNoise.cpp)

# glm
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)

# logging
set(SOURCES ${SOURCES} lib/loguru/loguru.hpp lib/loguru/loguru.cpp)

# procgen
add_subdirectory(procgen)
add_compile_definitions(${PROJECT_NAME} PROCGEN_BIN="${procgen_BINARY_DIR}/libprocgen.so")

# thread pool
add_subdirectory(threadpool)

# object pool
set(SOURCES ${SOURCES} lib/objectpool/src/object_pool.cpp lib/objectpool/src/object_pool.hpp)

# create library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# includes
target_include_directories(${PROJECT_NAME}
        PUBLIC
            src/
            lib/
            lib/objectpool/src
        )

# linkage
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(${PROJECT_NAME}
        PRIVATE
            Boost::system
            GLEW::GLEW
            SDL2::Main
            imgui
            jemalloc
        PUBLIC
            glm
            threadpool
)

# errors
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
