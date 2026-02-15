#include "shpch.h"

#ifdef SH_DEBUG
    #define VMA_LEAK_LOG_FORMAT(format, ...) do { \
            SH_ERROR((format), __VA_ARGS__); \
        } while(false)
#endif 

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>