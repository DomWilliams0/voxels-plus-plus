cmake_minimum_required(VERSION 3.14)
project(voxellib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -fno-stack-protector")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")


set(SOURCES
        game.cpp
        game.h
        world/world.cpp
        world/world.h
        error.h
        world/world_renderer.cpp
        world/world_renderer.h
        shader_loader.cpp
        shader_loader.h
        util.cpp
        util.h
        camera.cpp
        camera.h
        world/chunk.cpp world/chunk.h world/block.h
        lib/multidim_grid.hpp world/face.h world/face.cpp world/centre.h ui.cpp ui.h)


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

set(GLM_TEST_ENABLE off)
add_subdirectory(lib/glm)
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SOURCES_LIB})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${GLEW_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC lib/glm lib ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
