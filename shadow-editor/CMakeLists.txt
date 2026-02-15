cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SHADOW_EDITOR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SHADOW_EDITOR_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB SHADOW_EDITOR_PANELS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/panels/*.cpp)
file(GLOB SHADOW_EDITOR_PANELS_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/panels/*.h)

add_executable(Shadow-Editor ${SHADOW_EDITOR_SRC} ${SHADOW_EDITOR_INCLUDES}
                ${SHADOW_EDITOR_PANELS_SRC} ${SHADOW_EDITOR_PANELS_INCLUDES})

set_target_properties(Shadow-Editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY bin/shadow-editor/${OUTPUTCONFIG}
    LIBRARY_OUTPUT_DIRECTORY lib/shadow-editor/${OUTPUTCONFIG}
    ARCHIVE_OUTPUT_DIRECTORY lib/shadow-editor/${OUTPUTCONFIG}
)

target_include_directories(Shadow-Editor PRIVATE 
    ${CMAKE_SOURCE_DIR}/shadow/src
    ${SH_VENDOR_DIR}/glm
    ${SH_VENDOR_DIR}/assimp
    ${SH_VENDOR_DIR})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET Shadow-Editor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    add_definitions(-DSH_DEBUG)
elseif()
    set_property(TARGET Shadow-Editor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()