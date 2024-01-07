#pragma once

#include <Core/VkObjects/VkWindow.hpp>

namespace vkl
{
	template <class T>
	struct VarWithPrev
	{
		T current, prev;

		constexpr VarWithPrev():
			current(T(0)),
			prev(T(0))
		{}

		constexpr VarWithPrev(T const& t):
			current(t),
			prev(t)
		{}

		constexpr VarWithPrev& operator<<(T const& new_state)
		{
			prev = current;
			current = new_state;
			return *this;
		}

		constexpr T delta() const
		{
			return current - prev;
		}
	};

	template <class T>
	using VWP = VarWithPrev<T>;

	struct KeyState
	{
		int key;
		VWP<int> state = 0;

		KeyState(int key = 0) :
			key(key)
		{}

		bool currentlyPressed() const
		{
			return state.current == GLFW_PRESS;
		}

		bool currentlyReleased() const
		{
			return state.current == GLFW_RELEASE;
		}

		bool justPressed() const
		{
			return state.delta() > 0;
			//return state.current == GLFW_PRESS && state.prev == GLFW_RELEASE;
		}

		bool justReleased() const
		{
			return state.delta() < 0;
			//return state.current == GLFW_RELEASE && state.prev == GLFW_PRESS;
		}

		KeyState& operator<<(int new_state)
		{
			state << new_state;
			return *this;
		}
	};

	class InputListenerGLFW
	{
	protected:

		GLFWwindow* _window = nullptr;

	public:

		InputListenerGLFW(GLFWwindow * window):
			_window(window)
		{}

		virtual void update() = 0;
	};

	class KeyboardListener : public InputListenerGLFW
	{
	protected:

		std::vector<KeyState> _keys;

	public:

		KeyboardListener(GLFWwindow* window);

		virtual void update() override;


		const KeyState& getKey(int k)const
		{
			return _keys[k];
		}
	};

	class MouseListener : public InputListenerGLFW
	{
	protected:

		static glm::dvec2 s_scroll;

		static void mouseScrollCallback(GLFWwindow* window, double x, double y);

		VWP<glm::vec2> _scroll = glm::vec2(0);

		VWP<glm::vec2> _mouse_pos = glm::vec2(0);

		VWP<bool> _focus = false;
		VWP<bool> _hovered = false;

		std::vector<KeyState> _keys;

		// just preseed and just released
		std::vector<glm::vec2> _pressed_pos;
		std::vector<glm::vec2> _released_pos;

	public:

		MouseListener(GLFWwindow* window);

		virtual void update() override;

		const KeyState& getButton(int b)const
		{
			return _keys[b];
		}

		const VWP<glm::vec2>& getScroll()const
		{
			return _scroll;
		}

		const VWP<glm::vec2>& getPos()const
		{
			return _mouse_pos;
		}

		glm::vec2 getPressedPos(int b)const
		{
			return _pressed_pos[b];
		}

		glm::vec2 getReleasedPos(int b)const
		{
			return _released_pos[b];
		}
	};

	class GamepadListener : public InputListenerGLFW
	{
	protected:

		int _joystick = 0;
		std::vector<KeyState> _buttons;
		
		float _deadzone = 1e-1;
		std::vector<VWP<float>> _axis;

	public:

		GamepadListener(GLFWwindow* window, int jid);


		virtual void update() override;

		const KeyState& getButton(int b)const
		{
			return _buttons[b];
		}

		const VWP<float>& getAxis(int a)const
		{
			return _axis[a];
		}
	};
}