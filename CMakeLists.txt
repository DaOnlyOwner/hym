


cmake_minimum_required(VERSION 3.8)

macro(setupAndLink target)
target_compile_features(${target} PUBLIC cxx_std_20)
set_target_properties(${target} PROPERTIES CXX_STANDARD_REQUIRED ON)

target_compile_options(${target} PRIVATE -DUNICODE -DENGINE_DLL -DRES=\"${CMAKE_SOURCE_DIR}/resources\" -DSHADER_RES=\"${CMAKE_SOURCE_DIR}/resources/shaders\")
target_include_directories(${target} PRIVATE lib/DiligentCore engine/include lib/entt/single_include lib/DiligentTools/TextureLoader/interface)

target_link_libraries(${target} PRIVATE glfw)
target_link_libraries(${target}
PRIVATE
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-TextureLoader
    fmt
    glm
    assimp::assimp
)
copy_required_dlls(${target})
endmacro()


project(Hym)




set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HymTest)
file(GLOB_RECURSE SRC engine/src/*.cpp)
file(GLOB_RECURSE INC engine/include/*.h)

add_subdirectory(lib/DiligentCore)

option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_INSTALL "" OFF)
option(GLFW_BUILD_TESTS "" OFF)

add_subdirectory(lib/glfw)
add_subdirectory(lib/fmt)
add_subdirectory(lib/glm)
add_subdirectory(lib/assimp)
add_subdirectory(lib/DiligentTools)

add_executable(HymTest engine/test/test.cpp ${SRC} ${INC})
setupAndLink(HymTest)
