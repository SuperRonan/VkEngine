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
				_keys[k].key << new_state;
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
		_keys.resize(8);
		for (int k = 0; k < _keys.size(); ++k)
		{
			_keys[k].key = k;
		}
		glfwSetScrollCallback(_window, mouseScrollCallback);
	}

	void MouseListener::update()
	{
		_scroll << glm::vec2(s_scroll.x, s_scroll.y);
		s_scroll = glm::dvec2(0);

		for (auto& key : _keys)
		{
			const int new_state = glfwGetMouseButton(_window, key.key);
			key << new_state;
		}

		glm::dvec2 mouse_pos;
		glfwGetCursorPos(_window, &mouse_pos.x, &mouse_pos.y);
		_mouse_pos << glm::vec2(mouse_pos.x, mouse_pos.y);
	}
	
	
	GamepadListener::GamepadListener(GLFWwindow* window, int jid) :
		InputListenerGLFW(window),
		_joystick(jid)
	{
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
			_axis[a] << state.axes[a];
		}
	}

}