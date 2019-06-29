project(imgui)

add_library(${PROJECT_NAME} SHARED
        imconfig.h
        imgui.cpp
        imgui.h
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_impl_opengl3.cpp
        imgui_impl_opengl3.h
        imgui_impl_sdl.cpp
        imgui_impl_sdl.h
        imgui_internal.h
        imgui_widgets.cpp
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        )


find_package(SDL2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::Main)
target_compile_options(${PROJECT_NAME} PRIVATE -O0)
