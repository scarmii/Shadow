#pragma once

namespace Shadow
{
	class Timestep
	{
	public:
		Timestep(float time = 0.0f)
			: m_time(time)
		{
		}

		inline operator float() const { return m_time; } // return time in seconds
		inline float getMilliseconds() const { return m_time * 1000.0f; }
	private:
		float m_time;
	};
}