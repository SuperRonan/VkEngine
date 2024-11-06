#pragma once

#include <vkl/Rendering/RenderObjects.hpp>
#include <vkl/IO/GuiContext.hpp>
#include <vkl/IO/ImGuiUtils.hpp>

#include <vkl/Maths/Types.hpp>

#include <vkl/Maths/AlignedAxisBoundingBox.hpp>

namespace vkl
{
	class Camera : public VkObject
	{
	public:

		enum class Type
		{
			Perspective = 0,
			Orthographic = 1,
			ReversedPerspective = 2,
			Spherical = 3,
		};

		struct AsGLSL
		{
			vec3 position;
			float z_near;

			vec3 direction;
			float z_far;
			
			vec3 right;
			uint flags;
			
			float inv_tan_half_fov;
			float inv_aspect;
			float aperture;
			float focal_length;
		};
		

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

		float _aperture = 0;
		float _focal_length = 1;

		float _ortho_size = 1;

		bool _infinite_perspective = false;

		Type _type = Type::Perspective;

		ImGuiListSelection _gui_type;

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

		mat4 getCamToProj()const;

		mat4 getProjToCam()const;

		mat4x3 getWorldToCam() const
		{
			return LookAtDir(_position, _direction, up());
		}

		mat4x3 getCamToWorld() const
		{
			return InverseLookAtDir(_position, _direction, up());
		}

		mat4 getWorldToProj() const;

		mat4 GetProjToWorld() const;

		mat3 getWorldRoationMatrix() const
		{
			mat3 res = mat3(getWorldToCam());
			return res;
		}

		AABB3f getOrthoAABB() const
		{
			const vec3 lbn = vec3(
				-_ortho_size * _aspect,
				-_ortho_size,
				_near
			);
			const vec3 rtf = vec3(
				_ortho_size * _aspect,
				_ortho_size,
				_far
			);
			AABB3f res = AABB3f(lbn, rtf);
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
			return glm::cross(_direction, _right);
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

		constexpr float zNear() const
		{
			return _near;
		}

		constexpr float zFar() const
		{
			return _infinite_perspective ? std::numeric_limits<float>::infinity() : _far;
		}

		constexpr float aperture() const
		{
			return _aperture;
		}

		constexpr float focalLength() const
		{
			return _focal_length;
		}

		constexpr Type type() const
		{
			return _type;
		}

		// uv in clip space
		Ray getRay(vec2 uv = vec2(0)) const;

		void update(CameraDelta const& delta);

		void declareGui(GuiContext& ctx);

		AsGLSL getAsGLSL() const;

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