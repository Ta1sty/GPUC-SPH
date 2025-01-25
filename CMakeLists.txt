cmake_minimum_required(VERSION 3.23)
project(Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(compileShader TARGET SHADER DIM)
    find_program(GLSLC glslc)

    set(source ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(output ${CMAKE_CURRENT_SOURCE_DIR}/build/${SHADER}.${DIM}.spv)

    set(compile-script ${CMAKE_CURRENT_SOURCE_DIR}/compile_shader.cmake)

    add_custom_command(
            OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND} -DSOURCE=${source} -DOUTPUT=${output} -DDIM=${DIM} -P ${compile-script}
            DEPENDS ${source} shaders/_defines.glsl shaders/spatial_lookup.glsl
            VERBATIM
    )

    set_source_files_properties(${output} PROPERTIES GENERATED TRUE)

    target_sources(${TARGET} PRIVATE ${output})

endfunction(compileShader)

function(add_shader TARGET SHADER)

    compileShader(${TARGET} ${SHADER} 2D)
    compileShader(${TARGET} ${SHADER} 3D)

endfunction(add_shader)

add_subdirectory(libs)
find_package(Vulkan REQUIRED)

if (DEFINED ENV{RENDERDOC_PATH})
    set(RENDERDOC_PATH $ENV{RENDERDOC_PATH})
elseif (WIN32)
    if (EXISTS "C:\\Program Files\\RenderDoc")
        set(RENDERDOC_PATH "C:\\Program Files\\RenderDoc")
    endif ()
endif () # TODO LINUX

### Don't forget that there is a CMakeLists.txt in libs ! ###

set(IMGUI_DIR libs/imgui)
set(PROJECT_INCLUDE_DIRS
        include
        ${Vulkan_INCLUDE_DIRS}
        ${GLM_INCLUDE}
        ${GLFW_INCLUDE}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

set(SOURCES_IMGUI
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp)

set(SOURCES
        ${SOURCES_IMGUI}
        src/debug_image.cpp
        src/spatial_lookup.cpp
        src/host_timer.cpp
        src/imgui_ui.cpp
        src/initialization.cpp
        src/main.cpp
        src/obj.cpp
        src/particle_renderer.cpp
        src/particle_physics.cpp
        src/particles.cpp
        src/project.cpp
        src/renderdoc.cpp
        src/simulation.cpp
        src/stb.cpp
        src/task_common.cpp
        src/utils.cpp
        src/parameters.cpp
        src/simulation_state.cpp
        src/colormaps.cpp
        src/render.cpp
        src/camera.cpp
        src/debug.cpp
)

add_compile_definitions(WORKING_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${PROJECT_NAME} ${SOURCES})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

add_shader(${PROJECT_NAME} shaders/particle_simulation.comp)
add_shader(${PROJECT_NAME} shaders/density_update.comp)
add_shader(${PROJECT_NAME} shaders/position_update.comp)
add_shader(${PROJECT_NAME} shaders/particles.comp)
add_shader(${PROJECT_NAME} shaders/white.frag)
add_shader(${PROJECT_NAME} shaders/soup.vert)
add_shader(${PROJECT_NAME} shaders/phong.frag)
add_shader(${PROJECT_NAME} shaders/lines.vert)
add_shader(${PROJECT_NAME} shaders/particle.vert)
add_shader(${PROJECT_NAME} shaders/particle2d.vert)
add_shader(${PROJECT_NAME} shaders/particle3d.vert)
add_shader(${PROJECT_NAME} shaders/particle2d.geom)
add_shader(${PROJECT_NAME} shaders/particle2d.frag)
add_shader(${PROJECT_NAME} shaders/background2d.vert)
add_shader(${PROJECT_NAME} shaders/background2d.frag)
add_shader(${PROJECT_NAME} shaders/simulation_cube.vert)
add_shader(${PROJECT_NAME} shaders/ray_marcher.frag)
add_shader(${PROJECT_NAME} shaders/background_quad.vert)
add_shader(${PROJECT_NAME} shaders/background_environment.frag)
add_shader(${PROJECT_NAME} shaders/density_grid.comp)

add_shader(${PROJECT_NAME} shaders/spatial_lookup.write.comp)
add_shader(${PROJECT_NAME} shaders/spatial_lookup.sort.bubble.comp)
add_shader(${PROJECT_NAME} shaders/spatial_lookup.sort.bitonic.comp)
add_shader(${PROJECT_NAME} shaders/spatial_lookup.index.comp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan stb glfw tinyobj yaml-cpp)

if (RENDERDOC_PATH)
    target_include_directories(${PROJECT_NAME} PRIVATE ${RENDERDOC_PATH})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_RENDERDOC)
endif ()
