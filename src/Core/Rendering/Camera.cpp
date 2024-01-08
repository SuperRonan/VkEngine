#include "Camera.hpp"

namespace vkl
{
	Camera::Camera(CreateInfo const& ci) :
		_resolution_ifp(ci.resolution),
		_near(ci.znear),
		_far(ci.zfar)
	{

	}

	Ray Camera::getRay(vec2 uv) const
	{
		const vec2 cp = uvToClipSpace(uv);
		return Ray{
			.origin = _position,
			.direction = glm::normalize(_direction + cp.x * _right * _aspect - cp.y * _up),
		};
	}

	void Camera::update(CameraDelta const& delta)
	{
		const vec3 front = glm::cross(_up, _right);
		const float d = glm::dot(front, _direction);
		vec3 dp = delta.movement.x * _right + delta.movement.y * _up + delta.movement.z * front;

		dp = normalizeSafe(dp) * glm::length(delta.movement);
		_position += dp;

		_direction = rotate(_direction, _up, -delta.angle.x);
		computeInternal();

		const float y_angle = [&]() {
			const float current_cos_angle = glm::dot(_direction, _up);
			const float current_angle = std::acos(current_cos_angle);
			const float margin = 0.01;
			const float new_angle = std::clamp<float>(current_angle + delta.angle.y, margin * std::numbers::pi, (1.0f - margin) * std::numbers::pi);
			const float diff = new_angle - current_angle;
			//std::cout << glm::degrees(new_angle) << ", " << glm::degrees(diff) << ", " << glm::degrees(_fov) << std::endl;
			return diff;
		}();
		_direction = rotate(_direction, _right, -y_angle);
		//computeInternal();

		_fov *= delta.fov;
		_fov = std::clamp(_fov, glm::radians(1e-1f), glm::radians(179.0f));

		if (_resolution_ifp.hasValue())
		{
			VkExtent2D res = _resolution_ifp.value();
			_aspect = float(res.width) / float(res.height);
		}
	}

	void Camera::declareGui(GuiContext& ctx)
	{
		if (ImGui::CollapsingHeader("Camera"))
		{
			ImGui::SliderFloat("near plane", &_near, 0, _far);
			ImGui::SliderFloat("far plane", &_far, _near, 1e4 * _near);
		}
	}

	



	FirstPersonCameraController::FirstPersonCameraController(CreateInfo const& ci) :
		CameraController(ci.camera),
		_keyboard(ci.keyboard),
		_mouse(ci.mouse),
		_gamepad(ci.gamepad)
	{}

	void FirstPersonCameraController::updateCamera(float dt, MouseListener* mouse, KeyboardListener* keyboard, GamepadListener* gamepad)
	{
		Camera::CameraDelta delta;

		const float fov_sensitivity = (_camera->fov()) / (std::numbers::pi / 2.0);

		if(!mouse) mouse = _mouse;
		if(!keyboard) keyboard = _keyboard;
		if(!gamepad) gamepad = _gamepad;

		if (keyboard)
		{

			if (keyboard->getKey(_key_forward).currentlyPressed())
			{
				delta.movement.z += 1;
			}
			if (keyboard->getKey(_key_backward).currentlyPressed())
			{
				delta.movement.z += -1;
			}
			if (keyboard->getKey(_key_left).currentlyPressed())
			{
				delta.movement.x += -1;
			}
			if (keyboard->getKey(_key_right).currentlyPressed())
			{
				delta.movement.x += 1;
			}
			if (keyboard->getKey(_key_upward).currentlyPressed())
			{
				delta.movement.y += 1;
			}
			if (keyboard->getKey(_key_downward).currentlyPressed())
			{
				delta.movement.y += -1;
			}

			delta.movement = normalizeSafe(delta.movement);
		}



		if (mouse)
		{
			if (mouse->getButton(GLFW_MOUSE_BUTTON_LEFT).currentlyPressed())
			{
				delta.angle += mouse->getPos().delta() * _mouse_sensitivity;
			}

			delta.fov *= exp2(-mouse->getScroll().current.y * _fov_sensitivity);
		}

		if (gamepad)
		{
			delta.movement.x += gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_X).current;
			delta.movement.z -= gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_Y).current;

			if (gamepad->getButton(GLFW_GAMEPAD_BUTTON_CROSS).currentlyPressed())
			{
				delta.movement.y += 1;
			}
			if (gamepad->getButton(GLFW_GAMEPAD_BUTTON_CIRCLE).currentlyPressed())
			{
				delta.movement.y -= 1;
			}

			delta.angle.x += gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_X).current * dt * _joystick_sensitivity;
			delta.angle.y += gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y).current * dt * _joystick_sensitivity;

			float fov_zoom = gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER).current - gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER).current;
			delta.fov *= exp(fov_zoom * _fov_sensitivity * dt * 1e1);
		}

		delta.movement *= dt * _movement_speed;
		delta.angle *= fov_sensitivity;
		_camera->update(delta);
	}
}