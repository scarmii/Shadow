#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
		 
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>

#include "Shadow/Core/Log.hpp"
#include "Shadow/Debug/Instrumentor.hpp"

#ifdef SH_PLATFORM_WINDOWS
	#include <Windows.h>
#endif //MN_PLATFORM_WINDOWS