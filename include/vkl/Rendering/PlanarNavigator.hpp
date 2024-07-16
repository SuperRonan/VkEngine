#pragma once

#include <vkl/Maths/Types.hpp>
#include <vkl/IO/InputListener.hpp>

namespace vkl
{
	class PlanarNavigator
	{
	protected:

		using Float = float;
		using vec2 = Vector2<Float>;
		using mat3 = Matrix3<Float>;

		
		vec2 _translation;
		Float _zoom;


	public:

		PlanarNavigator(vec2 const t = vec2(0), Float zoom = 1);

		void update(Float screen_scale, MouseEventListener * mouse, KeyboardStateListener * keyboard, GamepadListener * gamepad);

		mat3 getWorldToView() const;


		Float getZoom()const
		{
			return _zoom;
		}

		vec2 getPosition() const
		{
			return _translation;
		}
	};
}