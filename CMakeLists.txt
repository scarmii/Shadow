cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

option(SH_PROFILE_RENDERER_FUNCTION "enables renderer profiling" off)
option(SH_UI_USE_DYNAMIC_RENDERING "imgui uses dynamic rendering extension" off)
option(RENDERER2D_INSTANCED "renderer 2d uses instanced rendering instead of batching" off)
option(RENDERER_STATISTICS "enables renderer statistics collection" off)
option(VULKAN_VALIDATION_LAYERS "enables vulkan validation layers (for non-debug builds)" off)

project(Shadow)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/${OUTPUTCONFIG})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib/${OUTPUTCONFIG})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib/${OUTPUTCONFIG})

set(SH_VENDOR_DIR ${CMAKE_SOURCE_DIR}/Shadow/vendor)
set(VULKAN_DIR C:/VulkanSDK/1.3.290.0/Include)
set(VULKAN_LIB C:/VulkanSDK/1.3.290.0/Lib)
set(ASSIMP_LIB ${SH_VENDOR_DIR}/assimp/lib/Release)

add_subdirectory(${CMAKE_SOURCE_DIR}/Shadow)
add_subdirectory(${CMAKE_SOURCE_DIR}/Sandbox)
add_subdirectory(${SH_VENDOR_DIR}/GLFW)
add_subdirectory(${SH_VENDOR_DIR}/SPIRV-Cross)
add_subdirectory(${SH_VENDOR_DIR}/imgui)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR}/Sandbox PROPERTY VS_STARTUP_PROJECT Sandbox)

target_include_directories(Shadow PUBLIC
    ${SH_VENDOR_DIR}
    ${CMAKE_SOURCE_DIR}/Shadow/src
    ${VULKAN_DIR}
    ${SH_VENDOR_DIR}/glm
    ${SH_VENDOR_DIR}/SPIRV-Cross
    ${SH_VENDOR_DIR}/imgui
    ${SH_VENDOR_DIR}/assimp/include
    ${SH_VENDOR_DIR}/GLFW/include)

target_link_libraries(Shadow GLFW SpirvCross ImGui ${VULKAN_LIB}/glslang.lib ${VULKAN_LIB}/vulkan-1.lib ${VULKAN_LIB}/volk.lib ${ASSIMP_LIB}/assimp-vc143-mt.lib)
target_link_libraries(Sandbox Shadow)

if(WIN32)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
endif()