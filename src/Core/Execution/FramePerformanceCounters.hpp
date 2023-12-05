#pragma once

namespace vkl
{
    struct FramePerfCounters
	{
		size_t update_time = 0;
		size_t render_time = 0;

		size_t prepare_scene = 0;
		size_t update_scene = 0;
		
		size_t generate_scene_draw_list = 0;
		size_t render_draw_list = 0;


		void reset()
		{
			memset(this, 0, sizeof(FramePerfCounters));
		}
	};
}