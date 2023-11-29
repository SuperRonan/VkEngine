#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Core/IO/InputListener.hpp>

#include <glm/ext/matrix_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <numbers>

#include <Core/Maths/Transforms.hpp>

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

	inline vec3 rotate(vec3 v, vec3 axis, float angle)
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
}