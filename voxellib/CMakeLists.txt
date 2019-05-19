cmake_minimum_required(VERSION 3.14)
project(voxellib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -fno-stack-protector -pipe -O0")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")


set(SOURCES
        src/game.cpp
        src/game.h
        src/world/world.cpp
        src/world/world.h
        src/error.h
        src/world/world_renderer.cpp
        src/world/world_renderer.h
        src/shader_loader.cpp
        src/shader_loader.h
        src/util.cpp
        src/util.h
        src/camera.cpp
        src/camera.h
        src/world/chunk.cpp
        src/world/chunk.h
        src/world/block.h
        src/world/face.h
        src/world/face.cpp
        src/world/centre.h
        src/ui.cpp
        src/ui.h
        lib/multidim_grid.hpp
        src/world/generation/generator.cpp src/world/generation/generator.h
        src/world/loader.cpp src/world/loader.h)
include_directories(src)


# imgui
set(SOURCES ${SOURCES}
        lib/imgui/imconfig.h
        lib/imgui/imgui.cpp
        lib/imgui/imgui.h
        lib/imgui/imgui_demo.cpp
        lib/imgui/imgui_draw.cpp
        lib/imgui/imgui_impl_opengl3.cpp
        lib/imgui/imgui_impl_opengl3.h
        lib/imgui/imgui_impl_sdl.cpp
        lib/imgui/imgui_impl_sdl.h
        lib/imgui/imgui_internal.h
        lib/imgui/imgui_widgets.cpp
        lib/imgui/imstb_rectpack.h
        lib/imgui/imstb_textedit.h
        lib/imgui/imstb_truetype.h
        )
include_directories(lib/imgui)


find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

set(GLM_TEST_ENABLE off)
add_subdirectory(lib/glm)
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SOURCES_LIB})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${GLEW_LIBRARIES} Boost::thread)
target_include_directories(${PROJECT_NAME} PUBLIC lib/glm lib ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
