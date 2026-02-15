cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ImGui STATIC
        imconfig.h
		imgui.h
		imgui.cpp
		imgui_draw.cpp
		imgui_internal.h
		imgui_tables.cpp
		imgui_widgets.cpp
		imstb_rectpack.h
		imstb_textedit.h
		imstb_truetype.h
		imgui_demo.cpp
        backends/imgui_impl_vulkan.h
		backends/imgui_impl_vulkan.cpp
		backends/imgui_impl_glfw.h
		backends/imgui_impl_glfw.cpp)

target_include_directories(ImGui PRIVATE 
	${SH_VENDOR_DIR}/GLFW/include 
	${CMAKE_CURRENT_SOURCE_DIR} 
	C:/VulkanSDK/1.3.290.0/Include)

set_target_properties(ImGui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY bin/ImGui/${OUTPUTCONFIG}
    LIBRARY_OUTPUT_DIRECTORY lib/ImGui/${OUTPUTCONFIG}
    ARCHIVE_OUTPUT_DIRECTORY lib/ImGui/${OUTPUTCONFIG}
)

if(WIN32)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
elseif()
    set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()