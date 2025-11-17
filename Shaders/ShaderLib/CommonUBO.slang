#pragma once

#include "common.slang"

// Follow SDL mouse buttons
#define MOUSE_BUTTON_LEFT 1
#define MOUSE_BUTTON_MIDDLE 2
#define MOUSE_BUTTON_RIGHT 3
#define MOUSE_BUTTON_X1 4
#define MOUSE_BUTTON_X2 5

#include "CommonUBO.h"

typealias MousePosPix = int16_t2;

struct MouseButtonState
{
	bool current_state;
	bool prev_state;

	property bool pressed
	{
		get {return current_state && !prev_state; }
	}

	property bool released
	{
		get {return !current_state && prev_state; }
	}
};

extension CommonUBO
{

	MousePosPix getLatestButtonMousePos(uint button, bool released)
	{
		return this.latest_mouse_button_pos_pix[button * 2 + (released ? 1 : 0)];
	}

	MouseButtonState getMouseButtonState(uint button)
	{
		MouseButtonState res;
		res.current_state = ((this.mouse_buttons_state >> (button * 2 + 0)) & 0x1) != 0;
		res.prev_state = ((this.mouse_buttons_state >> (button * 2 + 1)) & 0x1) != 0;
		return res;
	}
};

layout(COMMON_DESCRIPTOR_BINDING)
ConstantBuffer<CommonUBO, Std430DataLayout> _common_ubo;

