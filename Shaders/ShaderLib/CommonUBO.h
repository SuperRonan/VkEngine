#pragma once

#define COMMON_UBO_MAX_MOUSE_BUTTON 8

struct CommonUBO
{
	uint32_t debug_buffer_max_strings_mask;
	uint32_t debug_buffer_max_chunks_mask;
	uint32_t debug_buffer_max_lines_mask;
	uint32_t pad;

	uint64_t absolute_time_tick_64;
	uint64_t time_tick_64;

	uint64_t absolute_frame_idx_64;
	uint64_t frame_idx_64;

	float32_t time;
	float32_t delta_time;

	// w.r.t. the main window
	int16_t2 mouse_pos_pix;
	int16_t2 prev_mouse_pos_pix;
	
	// mouse_button * 2 + [pressed, released]
	int16_t2 latest_mouse_button_pos_pix[COMMON_UBO_MAX_MOUSE_BUTTON * 2];
	// bit-field: mouse_button * 2 + [current_state, prev_state]
	uint32_t mouse_buttons_state;
	
	float16_t2 mouse_scroll_delta;
	float16_t2 mouse_scroll;

};
