#pragma once

#include <vkl/Maths/Types.hpp>

#include <that/math/Half.hpp>

namespace vkl
{
	class DebugRenderer;
	class MouseEventListener;

	using uint16_t2 = Vector<uint16_t, 2>;
	using int16_t2 = Vector<int16_t, 2>;
	using float16_t2 = Vector<that::math::float16_t, 2>;

#include <ShaderLib/CommonUBO.h>

	extern void FillCommonUBO(CommonUBO& ubo, DebugRenderer const& debugger);
	extern void FillCommonUBO(CommonUBO& ubo, MouseEventListener const& mouse);
	extern void FillCommonUBO(CommonUBO& ubo, double time, double dt, uint64_t absolute_tick, uint64_t tick, uint64_t absolute_frame_idx, uint64_t frame_idx);
	static inline void FillCommonUBO(CommonUBO& ubo, double time, double dt, uint64_t tick, uint64_t frame_idx)
	{
		FillCommonUBO(ubo, time, dt, tick, tick, frame_idx, frame_idx);
	}

} // namespace vkl
