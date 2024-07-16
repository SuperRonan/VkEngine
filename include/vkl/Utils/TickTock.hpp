#pragma once

#include <chrono>

namespace std
{
	template <class C>
	concept ClockLike = std::chrono::is_clock<C>::value;

	template <ClockLike Clock = std::chrono::high_resolution_clock>
	class TickTock
	{
	public:
		using Clock_t = Clock;
		using TimePoint = typename Clock::time_point;
		using Rep = typename Clock::rep;
		using Period = typename Clock::period;
		using Duration = typename Clock::duration;

	protected:
		TimePoint _tick = {};
		TimePoint _tock = {};

#if VKL_BUILD_ANY_DEBUG
		mutable Duration _duration = {};
#endif

	public:

		TickTock()
		{}

		TimePoint tick()
		{
			_tick = Clock::now();
			return _tick;
		}

		TimePoint tock()
		{
			_tock = Clock::now();
			return _tock;
		}

		Duration tockd()
		{
			tock();
			return duration();
		}

		Duration duration()const
		{
#if !VKL_BUILD_ANY_DEBUG
			Duration
#endif
			_duration = (_tock - _tick);
			return _duration;
		}
	};

	using TickTock_hrc = TickTock<std::chrono::high_resolution_clock>;
}