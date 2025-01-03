set(GLM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)

set(GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

set_property(TARGET glfw PROPERTY FOLDER "libs")

add_subdirectory(yaml-cpp)

### allows you to just put stb in the "target link library"
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb/)

add_library(tinyobj INTERFACE)
target_include_directories(tinyobj INTERFACE tiny_obj_loader/)
