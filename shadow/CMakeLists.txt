cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SH_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB SH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SH_SH_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/*.h)
file(GLOB SH_SH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/*.cpp)
file(GLOB SH_CORE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/core/*.h)
file(GLOB SH_CORE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/core/*.cpp)
file(GLOB SH_EVENTS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/events/*.h)
file(GLOB SH_EVENTS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/events/*.cpp)
file(GLOB SH_UI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/imGui/*.h)
file(GLOB SH_UI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/imGui/*.cpp)
file(GLOB SH_RENDERER_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/renderer/*.h)
file(GLOB SH_RENDERER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/renderer/*.cpp)
file(GLOB SH_VULKAN_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/vulkan/*.h)
file(GLOB SH_VULKAN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/vulkan/*.cpp)
file(GLOB SH_DEBUG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/debug/*.h)
file(GLOB SH_UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/utils/*.h)
file(GLOB SH_UTILS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/utils/*.cpp)
file(GLOB SH_SCENE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/scene/*.h)
file(GLOB SH_SCENE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/shadow/scene/*.cpp)

add_library(Shadow STATIC 
    ${SH_SRC} ${SH_INCLUDE} 
    ${SH_SH_SRC} ${SH_SH_INCLUDE} 
    ${SH_CORE_INCLUDE} ${SH_CORE_SRC}
    ${SH_EVENTS_INCLUDE} ${SH_EVENTS_SRC}
    ${SH_UI_INCLUDE} ${SH_UI_SRC}
    ${SH_RENDERER_INCLUDE} ${SH_RENDERER_SRC}
    ${SH_VULKAN_INCLUDE} ${SH_VULKAN_SRC}
    ${SH_DEBUG_INCLUDE}
    ${SH_UTILS_INCLUDE} ${SH_UTILS_SRC}
    ${SH_SCENE_INCLUDE} ${SH_SCENE_SRC})

set_target_properties(Shadow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY bin/Shadow/${OUTPUTCONFIG}
    LIBRARY_OUTPUT_DIRECTORY lib/Shadow/${OUTPUTCONFIG}
    ARCHIVE_OUTPUT_DIRECTORY lib/Shadow/${OUTPUTCONFIG})

target_precompile_headers(Shadow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/shpch.h ${CMAKE_CURRENT_SOURCE_DIR}/src/shpch.cpp)

if(WIN32)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
    add_definitions(-DSH_PLATFORM_WINDOWS)
    add_custom_command(TARGET Shadow PRE_BUILD COMMAND call ${CMAKE_SOURCE_DIR}/scripts/win-compileShaders.bat)
elseif(LINUX)
    add_definitions(-DSH_PLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DSH_PLATFORM_APPLE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET Shadow PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    add_definitions(-DSH_DEBUG)
elseif()
    set_property(TARGET Shadow PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

if (SH_PROFILE_RENDERER_FUNCTION)
    add_definitions(-DSH_PROFILE_RENDERER_FUNCTION)
endif()

if (RENDERER2D_INSTANCED)
    add_definitions(-DRENDERER2D_INSTANCED)
endif()

if (RENDERER_STATISTICS)
    add_definitions(-DRENDERER_STATISTICS)
endif()

if (VULKAN_VALIDATION_LAYERS)
    add_definitions(-DSH_VULKAN_VALIDATION_LAYERS)
endif()

if (SH_TRACE_FRAMEBUFFER_RESIZE)
    add_definitions(-DSH_TRACE_FRAMEBUFFER_RESIZE)
endif()
