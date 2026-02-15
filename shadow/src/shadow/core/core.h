#pragma once

#ifdef SH_DEBUG
	#define SH_ASSERT(condition, message, ...) if(!condition)\
	{_log("[ASSERTION FAILED]:", message, TEXT_COLOR_BRIGHT_MAGENTA, ##__VA_ARGS__);\
	 __debugbreak();\
	}
#else
	#define SH_ASSERT(condition, ...)
#endif 

#define SH_ON_EVENT_FN(function, eventType) [this](const eventType& e) {return function(e);}

#define SH_FLAG(type) bool operator&(type arg1, type arg2);\
type operator|(type arg1, type arg2);\
void operator|=(type& arg1, type arg2);\

#define SH_FLAG_DEF(type, intType) bool operator&(type arg1, type arg2)\
{ return static_cast<intType>(arg1) & static_cast<intType>(arg2); }\
void operator|=(type& arg1, type arg2)\
{ arg1 = arg1 | arg2; }\
type operator|(type arg1, type arg2)\
{ return static_cast<type>(static_cast<intType>(arg1) | static_cast<intType>(arg2)); }\

////////////// Vulkan //////////////////////////////////////////////////////////
#define VK_LOAD_FUNC(instance, func) (PFN_##func)vkGetInstanceProcAddr(instance, #func)
#define VK_CHECK_RESULT(funcCall) {VkResult result = funcCall;\
if(result != VK_SUCCESS)\
{_log("[vulkan error]:", "%s returned %i", TEXT_COLOR_BRIGHT_RED, #funcCall, result);\
__debugbreak();\
}\
}
#ifdef SH_DEBUG
	#define VK_TRACE(message) _log("[vulkan trace]:", message, TEXT_COLOR_CYAN)
	#define VK_WARN(message) _log("[vulkan warn]:", message, 	TEXT_COLOR_BRIGHT_YELLOW)
	#define VK_ERROR(message) _log("[vulkan error]:", message, TEXT_COLOR_BRIGHT_RED)
#else
	#define VK_TRACE(message) 
	#define VK_WARN(message) 
	#define VK_ERROR(message) 
#endif 
////////////////////////////////////////////////////////////////////////////////