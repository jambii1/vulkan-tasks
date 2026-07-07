cmake_minimum_required(VERSION 3.29)

project(VulkanTriangleApp)

find_package(Vulkan 1.4.335 REQUIRED)
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)
target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
target_compile_definitions(VulkanCppModule
  INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

set(APP_NAME triangle_app)
add_executable(${APP_NAME}
  main.cpp
  triangle_app.cpp
)
set_target_properties(${APP_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${APP_NAME}
)
set_target_properties(${APP_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${APP_NAME} PRIVATE Vulkan::cppm glm::glm glfw)
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${APP_NAME} PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

set(SHADERS_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_SOURCES ${SHADERS_SOURCES_DIR}/triangle.slang)
set(ENTRY_POINTS -entry vertMain -entry fragMain)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}/shaders)
set(SHADERS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}/shaders)

add_custom_command(
  OUTPUT ${SHADERS_OUTPUT_DIR}/triangle.spv
  COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
    -target spirv -profile spirv_1_4+spvRayQueryKHR -emit-spirv-directly
    -fvk-use-entrypoint-name ${ENTRY_POINTS}
    -o ${SHADERS_OUTPUT_DIR}/triangle.spv
  WORKING_DIRECTORY ${SHADERS_OUTPUT_DIR}
  DEPENDS ${SHADER_SOURCES}
  COMMENT "Compiling slang shaders"
  VERBATIM
)

add_custom_target(compile_shaders
  DEPENDS ${SHADERS_OUTPUT_DIR}/triangle.spv
)

add_dependencies(${APP_NAME} compile_shaders)
