#include <vkl/Rendering/PlanarNavigator.hpp>
#include <vkl/Maths/Transforms.hpp>

#include <vkl/Utils/stl_extension.hpp>

#include <vkl/Maths/AffineXForm.hpp>

namespace vkl
{
	PlanarNavigator::PlanarNavigator(vec2 const t, Float zoom):
		_translation(t),
		_zoom(zoom)
	{}

	void PlanarNavigator::update(Float screen_scale, MouseEventListener* mouse, KeyboardStateListener* keyboard, GamepadListener* gamepad)
	{
		vec2 delta = vec2::Zero();
		vec2 zoom_center; // TODO
		Float zoom_mult = 1.0f;

		if (mouse)
		{
			float scroll = mouse->getScroll().current.y();
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
		mat3 res = mat3::Identity();
		const mat3 s = mat3(DiagonalMatrix<2>(_zoom));
		const mat3 t = mat3(TranslationMatrix((-_translation).eval()));
		res = s * t;
		return res;
	}
}