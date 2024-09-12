#include <vkl/Rendering/PlanarNavigator.hpp>
#include <vkl/Maths/Transforms.hpp>

#include <vkl/Utils/stl_extension.hpp>

namespace vkl
{
	PlanarNavigator::PlanarNavigator(vec2 const t, Float zoom):
		_translation(t),
		_zoom(zoom)
	{}

	void PlanarNavigator::update(Float screen_scale, MouseEventListener* mouse, KeyboardStateListener* keyboard, GamepadListener* gamepad)
	{
		vec2 delta = vec2(0);
		vec2 zoom_center; // TODO
		Float zoom_mult = 1.0f;

		if (mouse)
		{
			float scroll = mouse->getScroll().current.y;
			zoom_mult *= std::exp2(scroll * 0.1);

			const auto& mouse_left = mouse->getButton(SDL_BUTTON_LEFT);
			if (mouse_left.currentlyPressed())
			{
				vec2 mouse_delta = mouse->getPos().delta();
				delta -= mouse_delta * (2.0f / screen_scale);
			}
		}

		if (keyboard)
		{

		}

		if (gamepad)
		{

		}

		if (zoom_mult != Float(1))
		{
			_zoom *= zoom_mult;

		}
		_translation += delta / _zoom;
	}

	PlanarNavigator::mat3 PlanarNavigator::getWorldToView() const
	{
		mat3 res = mat3(1);
		const mat3 s = ScalingMatrix<3, Float>(_zoom);
		const mat3 t = TranslationMatrix<3, Float>(-_translation);
		res = s * t;
		using namespace glm_operators;
		//std::cout << res << std::endl << std::endl;
		return res;
	}
}