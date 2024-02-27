#include "InputListener.hpp"
#include <SDL_mouse.h>
#include <SDL_keyboard.h>
#include <SDL_gamecontroller.h>

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
		const uint8_t * sdl_keys = SDL_GetKeyboardState(nullptr);
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

		int x, y;
		SDL_GetMouseState(&x, &y);
		_mouse_pos = glm::vec2(x, y);

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
		res |= event.type == SDL_MOUSEMOTION;
		res |= event.type == SDL_MOUSEBUTTONDOWN;
		res |= event.type == SDL_MOUSEBUTTONUP;
		res |= event.type == SDL_MOUSEWHEEL;
		return res;
	}

	void MouseEventListener::processEventAssumeRelevent(SDL_Event const& event) 
	{
		if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
		{
			if (event.button.button < _buttons.size())
			{
				_buttons[event.button.button].latest_event_value = event.button.state;
				glm::vec2 pos = glm::vec2(event.button.x, event.button.y);
				if (event.type == SDL_MOUSEBUTTONDOWN)
				{
					_buttons[event.button.button].pressed_pos = pos;
				}
				else
				{
					_buttons[event.button.button].released_pos = pos;
				}
			}
		}
		else if (event.type == SDL_MOUSEMOTION)
		{
			glm::vec2 pos = glm::vec2(event.motion.x, event.motion.y);
			glm::vec2 rel = glm::vec2(event.motion.xrel, event.motion.yrel);
			_latest_event_pos = pos;
			_event_motion_accumulation += rel;
		}
		else if (event.type == SDL_MOUSEWHEEL)
		{
			glm::vec2 event_scroll = glm::vec2(event.wheel.preciseX, event.wheel.preciseY);
			_event_scroll_accumulation += event_scroll;
		}
	}

	void MouseEventListener::update()
	{
		_focus = SDL_GetMouseFocus();

		int x, y;
		uint32_t buttons_bits = SDL_GetMouseState(&x, &y);
		// Compare with _latest_event_pos
		_mouse_pos << glm::vec2(x, y);
		
		_scroll << _event_scroll_accumulation;
		_mouse_motion << _event_motion_accumulation;

		for (int k = 0; k < _buttons.size(); ++k)
		{
			_buttons[k].key << _buttons[k].latest_event_value;
		}


		_event_motion_accumulation = glm::vec2(0);
		_event_scroll_accumulation = glm::vec2(0);		
	}
	

	SDL_GameController* GamepadListener::FindController()
	{
		SDL_GameController* res = nullptr;
		for (int i = 0; i < SDL_NumJoysticks(); i++) {
			if (SDL_IsGameController(i)) {
				res = SDL_GameControllerOpen(i);
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
			std::cout << "Connected to controller: " << SDL_GameControllerName(_sdl_handle) << std::endl;
		}

		_buttons.resize(SDL_CONTROLLER_BUTTON_MAX);
		for (int b = 0; b < _buttons.size(); ++b)
		{
			_buttons[b].state = 0;
		}
		_axis.resize(SDL_CONTROLLER_AXIS_MAX);
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
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
				res = true;
			break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERAXISMOTION:
				res = true;
			break;
		}
		return res;
	}

	void GamepadListener::processEventAssumeRelevent(SDL_Event const& event)
	{
		switch (event.type)
		{
		case SDL_CONTROLLERDEVICEADDED:
		{
			if (!_sdl_handle)
			{
				_sdl_handle = SDL_GameControllerOpen(event.cdevice.which);
				std::cout << "Connected to controller " << SDL_GameControllerName(_sdl_handle) << std::endl;
			}
		}
		break;
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			if (_sdl_handle && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(_sdl_handle)))
			{
				std::cout << "Lost Controller!" << std::endl; 
				SDL_GameControllerClose(_sdl_handle);
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
				int button_state = SDL_GameControllerGetButton(_sdl_handle, (SDL_GameControllerButton)b);
				_buttons[b] << button_state;
			}
			for (int a = 0; a < _axis.size(); ++a)
			{
				int value = SDL_GameControllerGetAxis(_sdl_handle, (SDL_GameControllerAxis)a);
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