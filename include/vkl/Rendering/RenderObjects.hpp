#pragma once

#include <vkl/Maths/Types.hpp>

namespace vkl
{
	struct Ray
	{
		Vector3f origin;
		Vector3f direction;

		Vector3f t(float t)const
		{
			return origin + t * direction;
		}

		Vector3f operator()(float t)const
		{
			return this->t(t);
		}
	};	
}