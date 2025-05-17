cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(GLFW_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLFW/glfw3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLFW/glfw3native.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw.rc.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/context.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/null_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/null_joystick.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/null_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/null_window.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window.c)

set(GLFW_LINUX_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/x11_init.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/x11_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/x11_window.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/xkb_unicode.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/posix_module.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/posix_time.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/posix_thread.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/posix_module.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/glx_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/egl_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/osmesa_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/linux_joystick.c)

set(GLFW_APPLE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa_init.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa_monitor.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa_window.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa_joystick.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cocoa_time.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nsgl_context.m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/posix_thread.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/posix_module.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osmesa_context.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/egl_context.c)

set(GLFW_WINDOWS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/win32_init.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_joystick.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_module.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_time.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_thread.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/win32_window.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/wgl_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/egl_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/osmesa_context.c)

if(WIN32)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
    add_library(GLFW STATIC ${GLFW_SRC} ${GLFW_WINDOWS_SRC})
    add_definitions(-D_GLFW_WIN32 -D_CRT_SECURE_NO_WARNINGS)
elseif(LINUX)
    add_library(GLFW STATIC ${GLFW_SRC} ${GLFW_LINUX_SRC})
    add_definitions(-D_GLFW_X11)
elseif(APPLE)
    add_library(GLFW STATIC ${GLFW_SRC} ${GLFW_APPLE_SRC})
    add_definitions(-D_GLFW_COCOA)
endif()

set_target_properties(GLFW PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY bin/GLFW/${OUTPUTCONFIG}
    LIBRARY_OUTPUT_DIRECTORY lib/GLFW/${OUTPUTCONFIG}
    ARCHIVE_OUTPUT_DIRECTORY lib/GLFW/${OUTPUTCONFIG})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET GLFW PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
elseif()
    set_property(TARGET GLFW PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()