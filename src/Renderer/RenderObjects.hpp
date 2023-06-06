#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE


#include <glm/ext/matrix_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

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
		float _fov = 90.0;
		float _near = 0.1;
		float _far = 10.0;

	public:

		constexpr Camera() = default;

		mat4 getCamToProj()const
		{
			return glm::perspective(_fov, _aspect, _near, _far);
		}

		mat4 getWorldToCam() const
		{
			return glm::lookAt(_position, _position + _direction, _up);
		}

		mat4 getWorldToProj() const
		{
			return getCamToProj() * getWorldToCam();
		}

		void computeInternal()
		{
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

		Ray getRay(vec2 uv = vec2(0))
		{
			const vec2 cp = uvToClipSpace(uv);
			return Ray{
				.origin = _position,
				.direction = glm::normalize(_direction + cp.x * _right - cp.y * _up),
			};
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

		virtual void updateCamera() = 0;

	};

	class FirstPersonCameraController : public CameraController
	{
	protected:


	public:
		
		FirstPersonCameraController(Camera * camera):
			CameraController(camera)
		{}

		virtual void updateCamera() override
		{

		}
	};
}