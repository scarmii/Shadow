#pragma once

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef SH_DEBUG
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
