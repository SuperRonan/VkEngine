#pragma once

#include <vkl/Rendering/RenderObjects.hpp>
#include <vkl/IO/GuiContext.hpp>
#include <vkl/IO/ImGuiUtils.hpp>

#include <vkl/Maths/Types.hpp>

#include <vkl/Maths/View.hpp>

#include <vkl/Maths/AlignedAxisBoundingBox.hpp>

namespace vkl
{
	class Camera : public VkObject
	{
	public:
		
		using vec2 = Vector2f;
		using vec3 = Vector3f;
		using mat3 = Matrix3f;
		using mat4 = Matrix4f;
		using mat3x4 = Matrix3x4f;

		enum class Type
		{
			Perspective = 0,
			Orthographic = 1,
			//ReversedPerspective = 2,
			Spherical = 2,
		};

		struct AsGLSL
		{
			ubo_vec3 position;
			float z_near;

			vec3 direction;
			float z_far;
			
			vec3 right;
			uint flags;
			
			float inv_tan_half_fov;
			float inv_aspect;
			float aperture; // radius in m
			float focal_distance; // in m

		};
		

	protected:

		vec3 _position = vec3::Zero();
		vec3 _direction = vec3(0, 0, 1);
		vec3 _right = vec3(1, 0, 0);
		vec3 _up = vec3(0, 1, 0);

		DynamicValue<VkExtent2D> _resolution_ifp = {};

		float _aspect = 16.0 / 9.0;
		float _fov = Radians(90.0f);
		float _near = 0.1;
		float _far = 10.0;

		// diameter in mm
		float _aperture = 0;
		// in m
		float _focal_distance = 1;

		float _ortho_size = 1;

		bool _infinite_far = false;
		bool _reverse_depth = false;

		Type _type = Type::Perspective;

		ImGuiListSelection _gui_type;

	public:

		struct CameraDelta
		{
			vec3 movement = vec3::Zero();
			vec2 angle = vec2::Zero();
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

		mat3x4 getWorldToCam() const
		{
			return LookAtDir(_position, _direction, up());
		}

		mat3x4 getCamToWorld() const
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
			_direction = Normalize(_direction);
			_right = Normalize(Cross(_direction, _up));
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

		vec3 up()const
		{
			return Cross(_direction, _right);
		}

		float inclination()const
		{
			return std::acos(Dot(_direction, _up));
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
			return _infinite_far ? std::numeric_limits<float>::infinity() : _far;
		}

		constexpr float aperture() const
		{
			return _aperture;
		}

		constexpr float aperatureRadiusUnit() const
		{
			return aperture() / 2e3f;
		}

		constexpr float focalDistance() const
		{
			return _focal_distance;
		}

		constexpr float distanceFilmLens() const
		{
			return rcp(TanHalfFOV(_fov));
		}

		// in meters
		constexpr float focalLength() const
		{
			return rcp(TanHalfFOV(_fov) + rcp(focalDistance()));
		}

		constexpr float fNumber() const
		{
			return focalLength() / (aperture() / 1000.0f);
		}

		constexpr Type type() const
		{
			return _type;
		}

		bool hasReverseDepth() const
		{
			return _reverse_depth;
		}

		void setReverseDepth(bool reverse)
		{
			_reverse_depth = reverse;
		}

		// uv in clip space
		Ray getRay(vec2 uv = vec2::Zero()) const;

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