
#include <vkl/Rendering/Camera2D.hpp>

namespace vkl
{
	Camera2D::Camera2D() :
		_translate(0.f, 0.f),
		_zoom(1.f),
		_zoom_matrix(1.f)
	{}

	// pos in screen space
	Camera2D::Vector2 Camera2D::operator()(Vector2 const& pos)const
	{
		return _translate + vkl::deHomogenize<2, Float>(_zoom_matrix * vkl::homogenize<2, Float>(pos));
	}

	Camera2D::Matrix3 Camera2D::screenToWorldMatrix()const
	{
		return vkl::translateMatrix<3, Float>(_translate) * _zoom_matrix;
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

		Vector2 camera_pos = vkl::deHomogenize<2, Float>(_zoom_matrix * vkl::homogenize<2, Float>(screen_pos));

		Float mult = scroll > 0 ? (1.0f / (1.0f + scroll * _ds)) : ((-scroll * _ds + 1.0f));

		_zoom *= mult;

		Vector2 compensate_vector = camera_pos * (1 - mult);
		Matrix3 compensate_matrix = vkl::translateMatrix<3, Float>(compensate_vector);

		Matrix3 mult_matrix = vkl::scaleMatrix<3, Float>({ mult, mult });

		_zoom_matrix = compensate_matrix * mult_matrix * _zoom_matrix;
	}

	void Camera2D::reset()
	{
		_translate = { 0, 0 };
		_zoom = 1;
		_zoom_matrix = Matrix3(1);
	}
}