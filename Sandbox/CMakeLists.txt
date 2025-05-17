cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SANDBOX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SANDBOX_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

add_executable(Sandbox ${SANDBOX_SRC} ${SANDBOX_HEADERS})

set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY bin/Sandbox/${OUTPUTCONFIG}
    LIBRARY_OUTPUT_DIRECTORY lib/Sandbox/${OUTPUTCONFIG}
    ARCHIVE_OUTPUT_DIRECTORY lib/Sandbox/${OUTPUTCONFIG}
)

target_include_directories(Sandbox PRIVATE 
    ${CMAKE_SOURCE_DIR}/Shadow/src
    ${SH_VENDOR_DIR}/glm
    ${SH_VENDOR_DIR}/assimp
    ${SH_VENDOR_DIR})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET Sandbox PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    add_definitions(-DSH_DEBUG)
elseif()
    set_property(TARGET Sandbox PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()