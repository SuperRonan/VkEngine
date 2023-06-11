#pragma once

#include <glm/glm.hpp>
#include "Transforms.hpp"

namespace vkl
{
	class Camera2D
	{
	protected:

		// Screen space: something like [0, 1[ x [0, 1[ (not centered in zero)
		using Float = float;
		using Vector2 = glm::vec2;
		using Vector3 = glm::vec3;
		using Matrix3 = glm::mat3;

		Float _ds = 0.1f;

		Vector2 _translate;

		Float _zoom;

		//Screen space to camera window
		Matrix3 _zoom_matrix;

	public:

		Camera2D();

		// pos in screen space
		Vector2 operator()(Vector2 const& pos)const;

		Matrix3 matrix()const;

		Matrix3 screenToWorldMatrix()const;

		void move(Vector2 const& screen_delta);

		// Assumes scroll is not 0
		void zoom(Vector2 const& screen_pos, Float scroll);

		void reset();
	};
}