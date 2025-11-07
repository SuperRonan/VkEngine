#pragma once

namespace vkl
{
	struct FramePerfCounters
	{
		size_t frame_time = 0;

		size_t update_time = 0;
		size_t render_time = 0;

		size_t prepare_scene_time = 0;
		size_t update_scene_time = 0;
		size_t exec_update_time = 0;
		size_t main_script_modules_time = 0;
		size_t descriptor_updates = 0;
		
		size_t generate_scene_draw_list_time = 0;
		size_t render_draw_list_time = 0;
		
		size_t draw_calls = 0;
		size_t total_draw_calls = 0;
		size_t dispatch_calls = 0;
		size_t total_dispatch_threads = 0;

		size_t transfer_calls = 0;
		size_t total_upload_size = 0;

		size_t pipeline_barriers = 0;
		size_t buffer_barriers = 0;
		size_t image_barriers = 0;
		size_t layout_transitions = 0;

		void reset()
		{
			memset(this, 0, sizeof(FramePerfCounters));
		}
	};
}