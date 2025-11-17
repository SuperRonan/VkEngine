#include <vkl/Execution/CommonUBO.hpp>

#include <vkl/Rendering/DebugRenderer.hpp>

#include <vkl/IO/InputListener.hpp>

namespace vkl
{
	void FillCommonUBO(CommonUBO& ubo, DebugRenderer const& debugger)
	{
		const auto limits = debugger.getLimits();
		ubo.debug_buffer_max_strings_mask = limits.max_debug_strings_mask;
		ubo.debug_buffer_max_chunks_mask = limits.max_chunks_mask;
		ubo.debug_buffer_max_lines_mask = limits.max_lines_mask;
	}

	void FillCommonUBO(CommonUBO& ubo, MouseEventListener const& mouse)
	{
		auto pack = [](Vector2f pos)
		{
			int16_t2 res = pos.cast<int16_t>();
			return res;
		};
		ubo.mouse_pos_pix = pack(mouse.getPos().current);
		ubo.prev_mouse_pos_pix = pack(mouse.getPos().prev);

		ubo.mouse_buttons_state = 0;
		for (uint button = 0; button < std::min(uint(COMMON_UBO_MAX_MOUSE_BUTTON), uint(mouse.buttons().size())); ++button)
		{
			auto k = mouse.getButton(button);
			uint32_t bits = 0;
			if (k.state.current)
			{
				bits |= 0b01;
			}
			if (k.state.prev)
			{
				bits |= 0b10;
			}
			ubo.mouse_buttons_state |= (bits << (button * 2));

			ubo.latest_mouse_button_pos_pix[button * 2 + 0] = pack(mouse.getPressedPos(button));
			ubo.latest_mouse_button_pos_pix[button * 2 + 1] = pack(mouse.getReleasedPos(button));
		}

		ubo.mouse_scroll_delta = mouse.getScroll().delta().cast<that::math::float16_t>();
		ubo.mouse_scroll = mouse.getScroll().current.cast<that::math::float16_t>();
	}

	void FillCommonUBO(CommonUBO& ubo, double time, double dt, uint64_t absolute_tick, uint64_t tick, uint64_t absolute_frame_idx, uint64_t frame_idx)
	{
		ubo.absolute_time_tick_64 = absolute_tick;
		ubo.time_tick_64 = tick;
		ubo.absolute_frame_idx_64 = absolute_frame_idx;
		ubo.frame_idx_64 = frame_idx;

		ubo.time = static_cast<float32_t>(time);
		ubo.delta_time = static_cast<float32_t>(dt);
	}
}