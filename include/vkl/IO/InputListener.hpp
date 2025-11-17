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

		VWP<Vector2f> _scroll = Vector2f::Zero().eval();
		Vector2f _event_scroll_accumulation = Vector2f::Zero().eval();
		
		VWP<Vector2f> _mouse_pos = Vector2f::Zero().eval();
		VWP<Vector2f> _mouse_motion = Vector2f::Zero().eval();
		Vector2f _latest_event_pos = Vector2f::Zero();
		Vector2f _event_motion_accumulation = Vector2f::Zero();

		struct MyButtonState
		{
			KeyState key;
			Vector2f pressed_pos;
			Vector2f released_pos;
			int latest_event_value;
		};

		std::vector<MyButtonState> _buttons;

		SDL_Window * _focus = nullptr;

	public:

		MouseEventListener();

		virtual void update() override final;

		virtual bool eventIsRelevent(SDL_Event const& event) const override final;

		virtual void processEventAssumeRelevent(SDL_Event const& event) override final;

		const auto& buttons() const
		{
			return _buttons;
		}

		const KeyState& getButton(int b)const
		{
			return _buttons[b].key;
		}

		const VWP<Vector2f>& getScroll()const
		{
			return _scroll;
		}

		const VWP<Vector2f>& getPos()const
		{
			return _mouse_pos;
		}

		const Vector2f getMotion()const
		{
			return _mouse_motion.current;
		}

		Vector2f getPressedPos(int b)const
		{
			return _buttons[b].pressed_pos;
		}

		Vector2f getReleasedPos(int b)const
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