#pragma once

#include <vkl/Rendering/Camera.hpp>
#include <vkl/IO/InputListener.hpp>

namespace vkl
{
	class CameraController
	{
	protected:

		Camera* _camera;

	public:

		CameraController(Camera* camera) :
			_camera(camera)
		{
		}

		virtual void updateCamera(float dt, MouseEventListener* mouse = nullptr, KeyboardStateListener* keyboard = nullptr, GamepadListener* gamepad = nullptr) = 0;

	};

	class FirstPersonCameraController : public CameraController
	{
	protected:

		KeyboardStateListener* _keyboard = nullptr;
		MouseEventListener* _mouse = nullptr;
		GamepadListener* _gamepad = nullptr;

		int _key_forward = SDL_SCANCODE_W;
		int _key_backward = SDL_SCANCODE_S;
		int _key_left = SDL_SCANCODE_A;
		int _key_right = SDL_SCANCODE_D;
		int _key_upward = SDL_SCANCODE_SPACE;
		int _key_downward = SDL_SCANCODE_LCTRL;

		float _movement_speed = 1;
		float _mouse_sensitivity = 5e-3;
		float _joystick_sensitivity = 5;
		float _fov_sensitivity = 1e-1;

	public:

		struct CreateInfo
		{
			Camera* camera = nullptr;
			KeyboardStateListener* keyboard = nullptr;
			MouseEventListener* mouse = nullptr;
			GamepadListener* gamepad = nullptr;
		};

		FirstPersonCameraController(CreateInfo const& ci);

		int& keyUpward()
		{
			return _key_upward;
		}

		virtual void updateCamera(float dt, MouseEventListener* mouse = nullptr, KeyboardStateListener* keyboard = nullptr, GamepadListener* gamepad = nullptr) override;
	};
}