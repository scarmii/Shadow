#pragma once

#include <random>

namespace Shadow
{
	class Random
	{
	public:
		static void init()
		{
			s_RandomEngine.seed(std::random_device()());
		}

		static float _float()
		{
			return static_cast<float>(s_Distribution(s_RandomEngine)) /
				static_cast<float>(std::numeric_limits<uint32_t>::max());
		}
	private:
		static inline std::mt19937 s_RandomEngine;
		static inline std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
	};
}