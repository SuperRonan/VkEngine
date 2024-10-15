#pragma once

#include <vkl/VkObjects/VkWindow.hpp>
#include <SDL3/SDL_events.h>

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
		VWP<int> state = 0;

		KeyState() 
		{}

		bool currentlyPressed() const
		{
			// TODO
			return state.current != 0;
		}

		bool currentlyReleased() const
		{
			// TODO
			return state.current != 0;
		}

		bool justPressed() const
		{
			return state.delta() > 0;
			//return state.current == PRESSED && state.prev == RELEASED;
		}

		bool justReleased() const
		{
			return state.delta() < 0;
			//return state.current == RELEASED && state.prev == PRESSED;
		}

		KeyState& operator<<(int new_state)
		{
			state << new_state;
			return *this;
		}
	};

	// There are two types of input listeners:
	// State based: they query a SDL_Get"device"State each frame
	// Event based: they receive and process events to update their state
	// Is it really necessary to have to separate sub classes? 

	class InputListener
	{
	public:

		virtual void update() = 0;
	};

	class StateInputListener : public InputListener
	{
	public:
		
		virtual void update() override = 0;
	};

	class EventInputListener : public InputListener
	{
	public:

		virtual bool eventIsRelevent(SDL_Event const& event) const = 0;

		virtual void processEventAssumeRelevent(SDL_Event const& event) = 0;

		bool processEventCheckRelevent(SDL_Event const& event)
		{
			bool res = false;
			if (eventIsRelevent(event))
			{
				res = true;
				processEventAssumeRelevent(event);
			}
			return res;
		}

		virtual void update() override = 0;
	};


	class KeyboardStateListener : public StateInputListener
	{
	protected:

		static constexpr int MaxKey()
		{
			return SDL_SCANCODE_RGUI + 1;
		}

		std::vector<KeyState> _keys;
		SDL_Window * _focus = nullptr;

	public:

		KeyboardStateListener();

		virtual void update() override final;

		const KeyState& getKey(int k)const
		{
			assert(k < _keys.size());
			return _keys[k];
		}

		SDL_Window* focus()const
		{
			return _focus;
		}
	};

	// 
	class MouseEventListener : public EventInputListener
	{
	protected:

		VWP<glm::vec2> _scroll = glm::vec2(0);
		glm::vec2 _event_scroll_accumulation = glm::vec2(0);
		
		VWP<glm::vec2> _mouse_pos = glm::vec2(0);
		VWP<glm::vec2> _mouse_motion = glm::vec2(0);
		glm::vec2 _latest_event_pos = glm::vec2(0);
		glm::vec2 _event_motion_accumulation = glm::vec2(0);

		struct MyButtonState
		{
			KeyState key;
			glm::vec2 pressed_pos;
			glm::vec2 released_pos;
			int latest_event_value;
		};

		std::vector<MyButtonState> _buttons;

		SDL_Window * _focus = nullptr;

	public:

		MouseEventListener();

		virtual void update() override final;

		virtual bool eventIsRelevent(SDL_Event const& event) const override final;

		virtual void processEventAssumeRelevent(SDL_Event const& event) override final;

		const KeyState& getButton(int b)const
		{
			return _buttons[b].key;
		}

		const VWP<glm::vec2>& getScroll()const
		{
			return _scroll;
		}

		const VWP<glm::vec2>& getPos()const
		{
			return _mouse_pos;
		}

		const glm::vec2 getMotion()const
		{
			return _mouse_motion.current;
		}

		glm::vec2 getPressedPos(int b)const
		{
			return _buttons[b].pressed_pos;
		}

		glm::vec2 getReleasedPos(int b)const
		{
			return _buttons[b].released_pos;
		}

		SDL_Window* focus()const
		{
			return _focus;
		}
	};

	class GamepadListener : public EventInputListener
	{
	protected:

		SDL_Gamepad * _sdl_handle = nullptr;

		std::vector<KeyState> _buttons;
		
		float _deadzone = 1e-1;
		std::vector<VWP<float>> _axis;

	public:

		static SDL_Gamepad* FindController();

		GamepadListener();


		virtual void update() override;

		const KeyState& getButton(int b)const
		{
			return _buttons[b];
		}

		const VWP<float>& getAxis(int a)const
		{
			return _axis[a];
		}

		virtual bool eventIsRelevent(SDL_Event const& event) const override final;

		virtual void processEventAssumeRelevent(SDL_Event const& event) override final;
	};
}