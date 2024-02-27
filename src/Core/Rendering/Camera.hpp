#pragma once

#include <Core/Rendering/RenderObjects.hpp>
#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	class Camera : public VkObject
	{
	protected:

		vec3 _position = vec3(0);
		vec3 _direction = vec3(0, 0, 1);
		vec3 _right = vec3(1, 0, 0);
		vec3 _up = vec3(0, 1, 0);

		DynamicValue<VkExtent2D> _resolution_ifp = {};

		float _aspect = 16.0 / 9.0;
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

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			DynamicValue<VkExtent2D> resolution = {};
			float znear = 0.001;
			float zfar = 10;
		};
		using CI = CreateInfo;

		Camera(CreateInfo const& ci);

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

		mat4 getWorldRoationMatrix() const
		{
			mat4 res = getWorldToCam();
			res[3][0] = 0;
			res[3][1] = 0;
			res[3][2] = 0;
			return res;
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

		constexpr const vec3& right()const
		{
			return _right;
		}

		constexpr vec3 up()const
		{
			return glm::cross(_right, _direction);
		}

		float inclination()const
		{
			return std::acos(glm::dot(_direction, _up));
		}

		constexpr float fov()const
		{
			return _fov;
		}

		constexpr float aspect()const
		{
			return _aspect;
		}

		Ray getRay(vec2 uv = vec2(0)) const;

		void update(CameraDelta const& delta);

		void declareGui(GuiContext& ctx);

		friend class CameraController;
	};


	class CameraController
	{
	protected:

		Camera* _camera;

	public:

		CameraController(Camera* camera) :
			_camera(camera)
		{}

		virtual void updateCamera(float dt, MouseEventListener * mouse = nullptr, KeyboardStateListener * keyboard = nullptr, GamepadListener * gamepad = nullptr) = 0;

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