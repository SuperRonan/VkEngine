#include <vkl/IO/InputListener.hpp>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_gamepad.h>

namespace vkl
{
	KeyboardStateListener::KeyboardStateListener():
		StateInputListener()
	{
		_keys.resize(MaxKey());
		for (int k = 0; k < _keys.size(); ++k)
		{
			_keys[k].state = 0;
		}
	}

	void KeyboardStateListener::update()
	{
		const bool * sdl_keys = SDL_GetKeyboardState(nullptr);
		const auto queryKeys = [&](int begin, int end)
		{
			for (int k = begin; k < end; ++k)
			{
				_keys[k] << sdl_keys[k];
			}
		};
		queryKeys(0, MaxKey());
		_focus = SDL_GetKeyboardFocus();
	}


	MouseEventListener::MouseEventListener()
	{	
		_buttons.resize(SDL_BUTTON_X2 + 1);

		float x, y;
		SDL_GetMouseState(&x, &y);
		_mouse_pos = Vector2f(x, y);

		for (int k = 0; k < _buttons.size(); ++k)
		{
			_buttons[k].key.state = 0;
			_buttons[k].latest_event_value = 0;
			_buttons[k].pressed_pos = _mouse_pos.current;
			_buttons[k].released_pos = _mouse_pos.current;
		}
	}

	bool MouseEventListener::eventIsRelevent(SDL_Event const& event) const
	{
		bool res = false;
		res |= event.type == SDL_EVENT_MOUSE_MOTION;
		res |= event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
		res |= event.type == SDL_EVENT_MOUSE_BUTTON_UP;
		res |= event.type == SDL_EVENT_MOUSE_WHEEL;
		return res;
	}

	void MouseEventListener::processEventAssumeRelevent(SDL_Event const& event) 
	{
		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
		{
			if (event.button.button < _buttons.size())
			{
				int value = event.button.down ? 1 : 0;
				_buttons[event.button.button].latest_event_value = value;
				Vector2f pos = Vector2f(event.button.x, event.button.y);
				if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
				{
					_buttons[event.button.button].pressed_pos = pos;
				}
				else
				{
					_buttons[event.button.button].released_pos = pos;
				}
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			Vector2f pos = Vector2f(event.motion.x, event.motion.y);
			Vector2f rel = Vector2f(event.motion.xrel, event.motion.yrel);
			_latest_event_pos = pos;
			_event_motion_accumulation += rel;
		}
		else if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			Vector2f event_scroll = Vector2f(event.wheel.x, event.wheel.y);
			_event_scroll_accumulation += event_scroll;
		}
	}

	void MouseEventListener::update()
	{
		_focus = SDL_GetMouseFocus();

		float x, y;
		uint32_t buttons_bits = SDL_GetMouseState(&x, &y);
		// Compare with _latest_event_pos
		_mouse_pos << Vector2f(x, y);
		
		_scroll << _event_scroll_accumulation;
		_mouse_motion << _event_motion_accumulation;

		for (int k = 0; k < _buttons.size(); ++k)
		{
			_buttons[k].key << _buttons[k].latest_event_value;
		}


		_event_motion_accumulation = Vector2f::Constant(0);
		_event_scroll_accumulation = Vector2f::Constant(0);
	}
	

	SDL_Gamepad* GamepadListener::FindController()
	{
		int count;
		SDL_JoystickID * ids = SDL_GetGamepads(&count);
		SDL_Gamepad* res = nullptr;
		for (int i = 0; i < count; i++) {
			if (SDL_IsGamepad(ids[i])) {
				res = SDL_OpenGamepad(ids[i]);
				break;
			}
		}

		return res;
	}
	
	GamepadListener::GamepadListener() :
		_sdl_handle(nullptr)
	{
		_sdl_handle = FindController();

		if (_sdl_handle)
		{
			// TODO use logger
			std::cout << "Connected to controller: " << SDL_GetGamepadName(_sdl_handle) << std::endl;
		}

		_buttons.resize(SDL_GAMEPAD_BUTTON_COUNT);
		for (int b = 0; b < _buttons.size(); ++b)
		{
			_buttons[b].state = 0;
		}
		_axis.resize(SDL_GAMEPAD_AXIS_COUNT);
		for (int a = 0; a < _axis.size(); ++a)
		{
			_axis[a] = 0;
		}
	}

	bool GamepadListener::eventIsRelevent(SDL_Event const& event) const
	{
		
		bool res = false;
		switch (event.type)
		{
			case SDL_EVENT_GAMEPAD_ADDED:
			case SDL_EVENT_GAMEPAD_REMOVED:
				res = true;
			break;
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				res = true;
			break;
		}
		return res;
	}

	void GamepadListener::processEventAssumeRelevent(SDL_Event const& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			if (!_sdl_handle)
			{
				_sdl_handle = SDL_OpenGamepad(event.gdevice.which);
				// TODO use logger
				std::cout << "Connected to controller " << SDL_GetGamepadName(_sdl_handle) << std::endl;
			}
		}
		break;
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			if (_sdl_handle && event.gdevice.which == SDL_GetJoystickID(SDL_GetGamepadJoystick(_sdl_handle)))
			{
				// TODO use logger
				std::cout << "Lost Controller!" << std::endl; 
				SDL_CloseGamepad(_sdl_handle);
				_sdl_handle = FindController();
			}
		}
		break;
		}
	}

	void GamepadListener::update()
	{	
		if (_sdl_handle)
		{
			for (int b = 0; b < _buttons.size(); ++b)
			{
				int button_state = SDL_GetGamepadButton(_sdl_handle, (SDL_GamepadButton)b);
				_buttons[b] << button_state;
			}
			for (int a = 0; a < _axis.size(); ++a)
			{
				sint16_t value = SDL_GetGamepadAxis(_sdl_handle, (SDL_GamepadAxis)a);
				float f = float(value) / float(32768);
				if (std::abs(f) < _deadzone)
				{
					f = 0;
				}
				_axis[a] << f;
			}
		}
	}

}