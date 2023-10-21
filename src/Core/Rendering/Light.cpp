#include "Light.hpp"

namespace vkl
{
	LightGLSL LightGLSL::MakePoint(vec3 position, vec3 emission)
	{
		LightGLSL res{
				.position = position,
				.flags = LightType::POINT,
				.emission = emission,
		};
		return res;
	}

	LightGLSL LightGLSL::MakeDirectional(vec3 dir, vec3 emission)
	{
		LightGLSL res{
				.position = dir,
				.flags = LightType::DIRECTIONAL,
				.emission = emission,
		};
		return res;
	}

	LightGLSL LightGLSL::transform(mat4 const& mat) const
	{
		LightGLSL res;

		res.flags = flags;
		res.emission = emission;

		uint32_t type = flags;
		switch (type)
		{
		case LightType::POINT:
		{
			vec4 hpos = (mat * vec4(position, 1));
			res.position = hpos;
		}
		break;
		case LightType::DIRECTIONAL:
		{
			res.position = glm::normalize(directionMatrix(mat) * position);
		}
		break;
		}
		return res;
	}



	Light::Light(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type),
		_emission(ci.emission)
	{}


	PointLight::PointLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::POINT,
			.emission = ci.emission,
		}),
		_position(ci.position)
	{}

	LightGLSL PointLight::getAsGLSL(mat4 const& xform) const
	{
		const vec4 hpos = xform * vec4(_position, 1);
		LightGLSL res = LightGLSL::MakePoint(vec3(hpos), _emission);
		return res;
	}




	DirectionalLight::DirectionalLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::POINT,
			.emission = ci.emission,
		}),
		_direction(glm::normalize(ci.direction))
	{}

	LightGLSL DirectionalLight::getAsGLSL(mat4 const& xform) const
	{
		const vec3 dir = glm::normalize(directionMatrix(xform) * _direction);
		LightGLSL res = LightGLSL::MakeDirectional(dir, _emission);
		return res;
	}
}