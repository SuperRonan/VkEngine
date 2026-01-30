#include <vkl/Rendering/CameraController.hpp>

namespace vkl
{
	FirstPersonCameraController::FirstPersonCameraController(CreateInfo const& ci) :
		CameraController(ci.camera),
		_keyboard(ci.keyboard),
		_mouse(ci.mouse),
		_gamepad(ci.gamepad)
	{
	}

	void FirstPersonCameraController::updateCamera(float dt, MouseEventListener* mouse, KeyboardStateListener* keyboard, GamepadListener* gamepad)
	{
		Camera::CameraDelta delta;

		const float fov_sensitivity = (_camera->fov()) / (std::numbers::pi / 2.0);

		if (!mouse) mouse = _mouse;
		if (!keyboard) keyboard = _keyboard;
		if (!gamepad) gamepad = _gamepad;

		if (keyboard)
		{

			if (keyboard->getKey(_key_forward).currentlyPressed())
			{
				delta.movement[2] += 1;
			}
			if (keyboard->getKey(_key_backward).currentlyPressed())
			{
				delta.movement[2] += -1;
			}
			if (keyboard->getKey(_key_left).currentlyPressed())
			{
				delta.movement[0] += -1;
			}
			if (keyboard->getKey(_key_right).currentlyPressed())
			{
				delta.movement[0] += 1;
			}
			if (keyboard->getKey(_key_upward).currentlyPressed())
			{
				delta.movement[1] += 1;
			}
			if (keyboard->getKey(_key_downward).currentlyPressed())
			{
				delta.movement[1] += -1;
			}

			delta.movement = SafeNormalize(delta.movement);
		}



		if (mouse)
		{
			if (mouse->getButton(SDL_BUTTON_LEFT).currentlyPressed())
			{
				delta.angle += mouse->getPos().delta() * _mouse_sensitivity;
			}

			delta.fov *= std::exp2(-mouse->getScroll().current[1] * _fov_sensitivity);
		}

		if (gamepad)
		{
			delta.movement[0] += gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTX).current;
			delta.movement[2] -= gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTY).current;

			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_A).currentlyPressed())
			{
				delta.movement[1] += 1;
			}
			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_B).currentlyPressed())
			{
				delta.movement[1] -= 1;
			}

			delta.angle[0] += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTX).current * dt * _joystick_sensitivity;
			delta.angle[1] += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTY).current * dt * _joystick_sensitivity;

			float fov_zoom = gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER).current - gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER).current;
			delta.fov *= exp(fov_zoom * _fov_sensitivity * dt * 1e1);
		}

		delta.movement *= dt * _movement_speed;
		delta.angle *= fov_sensitivity;
		_camera->update(delta);
	}
}
