
#include <vkl/Rendering/Camera2D.hpp>
#include <vkl/Maths/AffineXForm.hpp>

namespace vkl
{
	Camera2D::Camera2D() :
		_translate(0.f, 0.f),
		_zoom(1.f),
		_zoom_matrix(DiagonalMatrix<3>(Float(1)))
	{}

	// pos in screen space
	Camera2D::Vector2 Camera2D::operator()(Vector2 const& pos)const
	{
		return _translate + HomogeneousNormalize(_zoom_matrix * Homogeneous(pos));
	}

	Camera2D::Matrix3 Camera2D::screenToWorldMatrix()const
	{
		return Matrix3(TranslationMatrix(_translate)) * _zoom_matrix;
	}

	Camera2D::Matrix3 Camera2D::matrix()const
	{
		return screenToWorldMatrix();
	}

	void Camera2D::move(Vector2 const& screen_delta)
	{
		_translate -= screen_delta * _zoom;
	}

	// Assumes scroll is not 0
	void Camera2D::zoom(Vector2 const& screen_pos, Float scroll)
	{
		assert(scroll != 0);

		Vector2 camera_pos = HomogeneousNormalize(_zoom_matrix * Homogeneous(screen_pos));

		Float mult = scroll > 0 ? (1.0f / (1.0f + scroll * _ds)) : ((-scroll * _ds + 1.0f));

		_zoom *= mult;

		Vector2 compensate_vector = camera_pos * (1 - mult);
		Matrix3 compensate_matrix = Matrix3(TranslationMatrix(compensate_vector));

		Matrix3 mult_matrix = Matrix3(ScalingMatrix(Vector2(mult, mult)));

		_zoom_matrix = compensate_matrix * (mult_matrix * _zoom_matrix).eval();
	}

	void Camera2D::reset()
	{
		_translate = { 0, 0 };
		_zoom = 1;
		_zoom_matrix = DiagonalMatrix<3, 3, Float>(1);
	}
}