#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Core/InputListener.hpp>

#include <glm/ext/matrix_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <numbers>

namespace vkl
{
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;

	constexpr vec2 clipSpaceToUv(vec2 cp)
	{
		return cp * 0.5f + vec2(0.5f);
	}

	constexpr vec2 uvToClipSpace(vec2 uv)
	{
		return uv * 2.0f - vec2(1);
	}

	template <glm::length_t N>
	glm::vec<N, float> normalizeSafe(glm::vec<N, float> v)
	{
		const float lv2 = glm::dot(v, v);
		if (lv2 != 0)
		{
			v = glm::normalize(v);
		}
		return v;
	}

	vec3 rotate(vec3 v, vec3 axis, float angle)
	{
		mat4 rotation = glm::rotate(mat4(1), angle, axis);

		vec4 res = rotation * vec4(v, 1);
		return vec3(res.x, res.y, res.z);
	}

	struct Ray
	{
		vec3 origin;
		vec3 direction;

		constexpr vec3 t(float t)const
		{
			return origin + t * direction;
		}

		constexpr vec3 operator()(float t)const
		{
			return this->t(t);
		}
	};


	class Camera
	{
	protected:

		vec3 _position = vec3(0);
		vec3 _direction = vec3(0, 0, 1);
		vec3 _right = vec3(1, 0, 0);
		vec3 _up = vec3(0, 1, 0);

		float _aspect = 16.0/9.0;
		float _fov = glm::radians(90.0);
		float _near = 0.1;
		float _far = 10.0;

	public:

		struct CameraDelta
		{
			vec3 movement = vec3(0);
			vec2 angle = vec2(0);
			float fov = 1;
		};

		constexpr Camera() = default;

		mat4 getCamToProj()const
		{
			mat4 res = glm::perspective(_fov, _aspect, _near, _far);
			res[1][1] *= -1;
			return res;
		}

		mat4 getWorldToCam() const
		{
			return glm::lookAt(_position, _position + _direction, glm::cross(_right, _direction));
		}

		mat4 getWorldToProj() const
		{
			return getCamToProj() * getWorldToCam();
		}

		void computeInternal()
		{
			_direction = glm::normalize(_direction);
			_right = glm::normalize(glm::cross(_direction, _up));
		}

		constexpr vec3 position()const
		{
			return _position;
		}

		constexpr vec3& position()
		{
			return _position;
		}

		constexpr vec3 direction() const
		{
			return _direction;
		}

		constexpr vec3& direction()
		{
			return _direction;
		}

		float inclination()const
		{
			return std::acos(glm::dot(_direction, _up));
		}

		constexpr float fov()const
		{
			return _fov;
		}

		Ray getRay(vec2 uv = vec2(0))
		{
			const vec2 cp = uvToClipSpace(uv);
			return Ray{
				.origin = _position,
				.direction = glm::normalize(_direction + cp.x * _right - cp.y * _up),
			};
		}

		void update(CameraDelta const& delta)
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
		}

		friend class CameraController;
	};

	class CameraController
	{
	protected:

		Camera * _camera;

	public:

		CameraController(Camera * camera):
			_camera(camera)
		{}

		virtual void updateCamera(float dt) = 0;

	};

	class FirstPersonCameraController : public CameraController
	{
	protected:

		KeyboardListener* _keyboard = nullptr;
		MouseListener* _mouse = nullptr;
		GamepadListener* _gamepad = nullptr;

		int _key_forward = GLFW_KEY_W;
		int _key_backward = GLFW_KEY_S;
		int _key_left = GLFW_KEY_A;
		int _key_right = GLFW_KEY_D;
		int _key_upward = GLFW_KEY_SPACE;
		int _key_downward = GLFW_KEY_LEFT_CONTROL;

		float _movement_speed = 1;
		float _mouse_sensitivity = 5e-1;
		float _joystick_sensitivity = 5;
		float _fov_sensitivity = 1e-1;

	public:

		struct CreateInfo
		{
			Camera* camera = nullptr;
			KeyboardListener* keyboard = nullptr;
			MouseListener* mouse = nullptr;
			GamepadListener* gamepad = nullptr;
		};
		
		FirstPersonCameraController(CreateInfo const& ci):
			CameraController(ci.camera),
			_keyboard(ci.keyboard),
			_mouse(ci.mouse),
			_gamepad(ci.gamepad)
		{}

		virtual void updateCamera(float dt) override
		{
			Camera::CameraDelta delta;

			const float fov_sensitivity = (_camera->fov()) / (std::numbers::pi / 2.0);

			if (_keyboard)
			{

				if (_keyboard->getKey(_key_forward).currentlyPressed())
				{
					delta.movement.z += 1;
				}
				if (_keyboard->getKey(_key_backward).currentlyPressed())
				{
					delta.movement.z += -1;
				}
				if (_keyboard->getKey(_key_left).currentlyPressed())
				{
					delta.movement.x += -1;
				}
				if (_keyboard->getKey(_key_right).currentlyPressed())
				{
					delta.movement.x += 1;
				}
				if (_keyboard->getKey(_key_upward).currentlyPressed())
				{
					delta.movement.y += 1;
				}
				if (_keyboard->getKey(_key_downward).currentlyPressed())
				{
					delta.movement.y += -1;
				}

				delta.movement = normalizeSafe(delta.movement);
			}

			
			
			if (_mouse)
			{
				if(_mouse->getButton(GLFW_MOUSE_BUTTON_RIGHT).currentlyPressed())
				{
					delta.angle += _mouse->getPos().delta() * dt * _mouse_sensitivity;
				}

				delta.fov *= exp(-_mouse->getScroll().current.y * _fov_sensitivity);
			}

			if (_gamepad)
			{
				delta.movement.x += _gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_X).current;
				delta.movement.z -= _gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_Y).current;

				if (_gamepad->getButton(GLFW_GAMEPAD_BUTTON_CROSS).currentlyPressed())
				{
					delta.movement.y += 1;
				}
				if (_gamepad->getButton(GLFW_GAMEPAD_BUTTON_CIRCLE).currentlyPressed())
				{
					delta.movement.y -= 1;
				}

				delta.angle.x += _gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_X).current * dt * _joystick_sensitivity;
				delta.angle.y += _gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y).current * dt * _joystick_sensitivity;

				float fov_zoom = _gamepad->getAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER).current - _gamepad->getAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER).current;
				delta.fov *= exp(fov_zoom * _fov_sensitivity * dt * 1e1);
			}

			delta.movement *= dt * _movement_speed;
			delta.angle *= fov_sensitivity;
			_camera->update(delta);
		}
	};
}