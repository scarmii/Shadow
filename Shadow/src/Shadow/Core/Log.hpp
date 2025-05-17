#pragma once

#include <cstdio>

//  #######################################################################
//                              Logging
//  #######################################################################

enum TextColor
{
	TEXT_COLOR_BLACK,
	TEXT_COLOR_RED,
	TEXT_COLOR_GREEN,
	TEXT_COLOR_YELLOW,
	TEXT_COLOR_BLUE,
	TEXT_COLOR_MAGENTA,
	TEXT_COLOR_CYAN,
	TEXT_COLOR_WHITE,
	TEXT_COLOR_BRIGHT_BLACK,
	TEXT_COLOR_BRIGHT_RED,
	TEXT_COLOR_BRIGHT_GREEN,
	TEXT_COLOR_BRIGHT_YELLOW,
	TEXT_COLOR_BRIGHT_BLUE,
	TEXT_COLOR_BRIGHT_MAGENTA,
	TEXT_COLOR_BRIGHT_CYAN,
	TEXT_COLOR_BRIGHT_WHITE,
	TEXT_COLOR_COUNT
};

template<typename ...Args>
static void _log(const char* prefix, const char* message, TextColor textColor, Args... args)
{
	static const char* TextColorTable[TEXT_COLOR_COUNT] =
	{
		"\x1b[30m", // TEXT_COLOR_BLACK
		"\x1b[31m", // TEXT_COLOR_RED
		"\x1b[32m", // TEXT_COLOR_GREEN
		"\x1b[33m", // TEXT_COLOR_YELLOW
		"\x1b[34m", // TEXT_COLOR_BLUE
		"\x1b[35m", // TEXT_COLOR_MAGENTA
		"\x1b[36m", // TEXT_COLOR_CYAN
		"\x1b[37m", // TEXT_COLOR_WHITE
		"\x1b[90m", // TEXT_COLOR_BRIGHT_BLACK
		"\x1b[91m", // TEXT_COLOR_BRIGHT_RED
		"\x1b[92m",	// TEXT_COLOR_BRIGHT_GREEN
		"\x1b[93m", // TEXT_COLOR_BRIGHT_YELLOW
		"\x1b[94m", // TEXT_COLOR_BRIGHT_BLUE
		"\x1b[95m", // TEXT_COLOR_BRIGHT_MAGENTA
		"\x1b[96m", // TEXT_COLOR_BRIGHT_CYAN
		"\x1b[97m", // TEXT_COLOR_BRIGHT_WHITE
	};

	char formatBuffer[2048] = {};
	sprintf(formatBuffer, "%s %s %s \033[0m", TextColorTable[textColor], prefix, message);

	char textBuffer[2048] = {};
	sprintf(textBuffer, formatBuffer, args...);

	puts(textBuffer);
}

//  #######################################################################
//                              Defines 
//  #######################################################################

#ifdef SH_DEBUG
	#define SH_TRACE(message,...) _log("[TRACE]:", message, TEXT_COLOR_BRIGHT_BLUE, ##__VA_ARGS__)
	#define SH_WARN(message, ...)  _log("[WARN]:", message, TEXT_COLOR_YELLOW, ##__VA_ARGS__)
	#define SH_ERROR(message, ...) _log("[ERROR]:", message, TEXT_COLOR_RED, ##__VA_ARGS__)
	#define SH_FATAL(message, ...) _log("[FATAL]:", message, TEXT_COLOR_BRIGHT_RED, ##__VA_ARGS__)

	#ifdef SH_USING_VULKAN
		#define SH_VULKAN_INFO(message, ...) _log("[VULKAN INFO]:", message, TEXT_COLOR_BRIGHT_BLUE, ##__VA_ARGS__)
		#define SH_VULKAN_WARN(message, ...) _log("[VULKAN WARN]:", message, TEXT_COLOR_BRIGHT_YELLOW, ##__VA_ARGS__)
		#define SH_VULKAN_ERROR(message, ...) _log("[VULKAN ERROR]:", message, TEXT_COLOR_BRIGHT_RED, ##__VA_ARGS__)
		#define SH_VULKAN_FATAL(message, ...) _log("[VULKAN FATAL]:", message, TEXT_COLOR_BRIGHT_RED, ##__VA_ARGS__)
	#endif 

	#ifdef SH_USING_OPENGL
		#define SH_OPENGL_INFO(message, ...) _Log("[OPENGL INFO]:", message, TEXT_COLOR_BRIGHT_BLUE, ##__VA_ARGS__)
		#define SH_OPENGL_WARN(message, ...) _Log("[OPENGL WARN]:", message, TEXT_COLOR_BRIGHT_YELLOW, ##__VA_ARGS__)
		#define SH_OPENGL_ERROR(message, ...) _Log("[OPENGL ERROR]:", message, TEXT_COLOR_BRIGHT_RED, ##__VA_ARGS__)
		#define SH_OPENGL_FATAL(message, ...) _Log("[OPENGL FATAL]:", message, TEXT_COLOR_BRIGHT_RED, ##__VA_ARGS__)
	#endif
#else
	#define SH_TRACE(message, ...)
	#define SH_WARN(message, ...)
	#define SH_ERROR(message, ...) 
	#define SH_FATAL(message, ...)

	#define SH_VULKAN_INFO(message, ...) 
	#define SH_VULKAN_WARN(message, ...)
	#define SH_VULKAN_ERROR(message, ...)
	#define SH_VULKAN_FATAL(message, ...)

	#define SH_OPENGL_INFO(message, ...)
	#define SH_OPENGL_WARN(message, ...) 
	#define SH_OPENGL_ERROR(message, ...) 
	#define SH_OPENGL_FATAL(message, ...) 
#endif 

