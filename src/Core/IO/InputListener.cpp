#include "InputListener.hpp"


namespace vkl
{
	KeyboardListener::KeyboardListener(GLFWwindow * window):
		InputListenerGLFW(window)
	{
		_keys.resize(GLFW_KEY_LAST + 1);
		for (int k = 0; k < _keys.size(); ++k)
		{
			_keys[k].key = k;
		}
	}

	void KeyboardListener::update()
	{
		struct KeyRange
		{
			int begin;
			int end; // included
		};

		std::array ranges = {
			KeyRange{.begin = GLFW_KEY_SPACE, .end = GLFW_KEY_GRAVE_ACCENT, },
			KeyRange{.begin = GLFW_KEY_ESCAPE, .end = GLFW_KEY_MENU, },
		};

		for (const auto range : ranges)
		{
			for (int k = range.begin; k < range.end; ++k)
			{
				const int new_state = glfwGetKey(_window, _keys[k].key);
				_keys[k] << new_state;
			}
		}
	}

	
	glm::dvec2 MouseListener::s_scroll = glm::dvec2(0);

	void MouseListener::mouseScrollCallback(GLFWwindow* window, double x, double y)
	{
		s_scroll = glm::dvec2(x, y);
	}

	MouseListener::MouseListener(GLFWwindow* window) :
		InputListenerGLFW(window)
	{
		
		_keys.resize(GLFW_MOUSE_BUTTON_LAST + 1);
		_pressed_pos.resize(_keys.size());
		_released_pos.resize(_keys.size());
		for (int k = 0; k < _keys.size(); ++k)
		{
			_keys[k].key = k;
		}
		glfwSetScrollCallback(_window, mouseScrollCallback);
	}

	void MouseListener::update()
	{
		_focus << glfwGetWindowAttrib(_window, GLFW_FOCUSED);
		if (_focus.current && !_focus.prev)
		{
			glm::dvec2 mouse_pos;
			glfwGetCursorPos(_window, &mouse_pos.x, &mouse_pos.y);
			_mouse_pos = glm::vec2(mouse_pos.x, mouse_pos.y);
		}
		if (_focus.current)
		{
			_scroll << glm::vec2(s_scroll.x, s_scroll.y);

			glm::dvec2 mouse_pos;
			glfwGetCursorPos(_window, &mouse_pos.x, &mouse_pos.y);
			_mouse_pos << glm::vec2(mouse_pos.x, mouse_pos.y);
		}
		s_scroll = glm::dvec2(0);


		for (int k = 0; k < _keys.size(); ++k)
		{
			auto& key = _keys[k];
			const int new_state = glfwGetMouseButton(_window, key.key);
			key << new_state;

			if (key.justPressed())
			{
				_pressed_pos[k] = _mouse_pos.current;
			}
			else if (key.justReleased())
			{
				_released_pos[k] = _mouse_pos.current;
			}
			
		}

	}
	
	
	GamepadListener::GamepadListener(GLFWwindow* window, int jid) :
		InputListenerGLFW(window),
		_joystick(jid)
	{
		if (glfwJoystickIsGamepad(_joystick))
		{
			std::cout << "Connected to jostick " << _joystick << ": " << glfwGetJoystickName(_joystick) << std::endl;
		}
		else
		{
			std::cout << "Failed to connect to jostick " << _joystick << std::endl;
		}
		_buttons.resize(GLFW_GAMEPAD_BUTTON_LAST + 1);

		for (int b = 0; b < _buttons.size(); ++b)
		{
			_buttons[b].key = b;
		}

		_axis.resize(GLFW_GAMEPAD_AXIS_LAST + 1);
	}

	void GamepadListener::update()
	{
		GLFWgamepadstate state;
		glfwGetGamepadState(_joystick, &state);
		for (int b = 0; b < _buttons.size(); ++b)
		{
			_buttons[b] << int(state.buttons[b]);
		}
		for (int a = 0; a < _axis.size(); ++a)
		{
			float ax = state.axes[a];
			if (std::abs(ax) < _deadzone)	ax = 0;
			_axis[a] << ax;
		}
	}

}