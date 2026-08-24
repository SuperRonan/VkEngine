#include <vkl/Rendering/Camera.hpp>
#include <vkl/Maths/Transforms.hpp>

#include <ShaderLib/Rendering/Camera/CameraDefinitions.h>

#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Panel.hpp>
#include <vkl/GUI/InspectorMakeInfo.hpp>

namespace vkl
{
	Camera::Camera(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_resolution_ifp(ci.resolution),
		_near(ci.znear),
		_far(ci.zfar)
	{
	
	}

	Ray Camera::getRay(vec2 uv) const
	{
		const vec2 cp = UVToClipSpace(uv);
		return Ray{
			.origin = _position,
			.direction = Normalize(_direction + cp[0] * _right * _aspect - cp[1] * _up),
		};
	}

	Camera::mat4 Camera::getCamToProj()const
	{
		mat4 res;
		if (_type == Type::Perspective)
		{
			if (_infinite_far || std::isinf(_far))
			{
				res = InfinitePerspectiveProjFromFOV(_fov, _aspect, _near, _reverse_depth);
			}
			else
			{
				res = PerspectiveProjFromFOV(_fov, _aspect, vec2(_near, _far), _reverse_depth);
			}
		}
		else if (_type == Type::Orthographic)
		{
			const AABB3f aabb = getOrthoAABB();
			res = OrthoProj(aabb.bottom(), aabb.top(), _reverse_depth);
		}
		return res;
	}

	Camera::mat4 Camera::getProjToCam() const
	{
		mat4 res;
		if (_type == Type::Perspective)
		{
			if (_infinite_far || std::isinf(_far))
			{
				res = InverseInfinitePerspectiveProjFromFOV(_fov, _aspect, _near, _reverse_depth);
			}
			else
			{
				res = InversePerspectiveProjFromFOV(_fov, _aspect, vec2(_near, _far), _reverse_depth);
			}
		}
		else if (_type == Type::Orthographic)
		{
			const AABB3f aabb = getOrthoAABB();
			res = InverseOrthoProj(aabb.bottom(), aabb.top(), _reverse_depth);
		}
		return res;
	}

	Camera::mat4 Camera::getWorldToProj() const
	{
		return getCamToProj() * mat4(getWorldToCam());
	}

	Camera::mat4 Camera::GetProjToWorld() const
	{
		return mat4(getCamToWorld() * getProjToCam());
	}

	void Camera::update(CameraDelta const& delta)
	{
		const vec3 front = Cross(_up, _right);
		const float d = Dot(front, _direction);
		mat3 basis = MakeFromCols(_right, _up, front);
		vec3 dp = basis * delta.movement;

		dp = SafeNormalize(dp) * Length(delta.movement);
		_position += dp;

		_direction = Rotate(_direction, _up, -delta.angle[0]);
		computeInternal();

		const float y_angle = [&]() {
			const float current_cos_angle = Dot(_direction, _up);
			const float current_angle = std::acos(current_cos_angle);
			const float margin = 0.01;
			const float new_angle = std::clamp<float>(current_angle + delta.angle[1], margin * std::numbers::pi, (1.0f - margin) * std::numbers::pi);
			const float diff = new_angle - current_angle;
			//std::cout << glm::degrees(new_angle) << ", " << glm::degrees(diff) << ", " << glm::degrees(_fov) << std::endl;
			return diff;
		}();
		_direction = Rotate(_direction, _right, -y_angle);
		//computeInternal();

		if (_type == Type::Perspective || _type == Type::Spherical)
		{
			_fov *= delta.fov;
			float max_fov = 180;
			if(_type == Type::Spherical) max_fov = 360;
			_fov = std::clamp(_fov, Radians(1e-1f), Radians(max_fov));
		}
		else if (_type == Type::Orthographic)
		{
			_ortho_size *= delta.fov;
			_ortho_size = std::max(1e-3f, _ortho_size);
		}

		if (_resolution_ifp.hasValue())
		{
			VkExtent2D res = _resolution_ifp.value();
			_aspect = float(res.width) / float(res.height);
		}
	}

	namespace GUI
	{
		class CameraInspector : public Panel
		{
		protected:

			std::shared_ptr<Camera> _target;

			ImGuiListSelection _type;
		public:
			
			CameraInspector(std::shared_ptr<Camera> const& target) :
				Panel(target->application(), std::format("{} - Camera Inspector###{}", target->name(), static_cast<const void*>(target.get()))),
				_target(target)
			{
				_type = ImGuiListSelection::CI{
					.mode = ImGuiListSelection::Mode::RadioButtons,
					.same_line = true,
					.labels = {
						"Perspective",
						"Orthographic",
						"Spherical"
					},
					.default_index = 0,
				};
			}

			virtual void declareInline(Context& ctx) override
			{
				ImGui::Checkbox("Infinite far", &_target->_infinite_far);
				ImGui::SameLine();
				ImGui::Checkbox("Reversed depth", &_target->_reverse_depth);
				float f;
				f = _target->_near;
				if (ImGui::SliderFloat("near plane", &f, 0, _target->_far))
				{
					_target->_near = f;
				}

				if (_target->_type != Camera::Type::Spherical)
				{
					f = _target->_far;
					if (ImGui::SliderFloat("far plane", &f, _target->_near, 1e4 * _target->_near))
					{
						_target->_far = f;
					}
				}

				_type.setIndex(static_cast<size_t>(_target->type()));
				if (_type.declare())
				{
					_target->_type = Camera::Type(_type.index());
				}

				if (std::IsAnyOf(_target->_type,Camera::Type::Perspective ,Camera::Type::Spherical))
				{
					f = _target->_fov;
					float max_fov = 180;
					if (_target->_type == Camera::Type::Spherical)	max_fov *= 2;
					if (ImGui::SliderAngle("FOV", &f, 0, max_fov))
					{
						_target->_fov = f;
					}
				}

				if (_target->_type == Camera::Type::Perspective)
				{
					ImGui::SliderFloat("Aperture", &_target->_aperture, 0, 100, "%.1f mm", ImGuiSliderFlags_NoRoundToFormat);
					ImGui::SliderFloat("Focal distance", &_target->_focal_distance, 0, 100, "%.3f m", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_Logarithmic);

					float f = _target->focalLength() * 1e3f;
					if (ImGui::InputFloat("Focal Length", &f, 0, 0, "%.1f mm", ImGuiInputTextFlags_EnterReturnsTrue & 0))
					{
						// Auto focus
						float fm = f / 1e3f;
						_target->_focal_distance = fm * _target->distanceFilmLens() * rcp(abs(fm - _target->distanceFilmLens()));
					}
					ImGui::Text("f-number: f / %.1f", _target->fNumber());
				}
				else if (_target->_type == Camera::Type::Orthographic)
				{

				}
			}
		};
	}

	std::shared_ptr<GUI::Panel> Camera::makeInspector(GUI::InspectorMakeInfo const& imi)
	{
		assert(imi.target.get() == this);
		return std::make_shared<GUI::CameraInspector>(std::static_pointer_cast<Camera>(imi.target));
	}

	Camera::AsGLSL Camera::getAsGLSL() const
	{
		AsGLSL res{
			.position = _position,
			.z_near = _near,
			.direction = _direction,
			.z_far = zFar(),
			.right = _right,
			.flags = 0,
			.inv_tan_half_fov = rcp(TanHalfFOV(_fov)),
			.inv_aspect = rcp(_aspect),
			.aperture = aperatureRadiusUnit(),
			.focal_distance = _focal_distance,
		};
		uint32_t type = 0;
		if (_type == Type::Perspective)
		{
			type = CAMERA_TYPE_PERSPECTIVE;
			if (_aperture > 0)
			{
				type = CAMERA_TYPE_THIN_LENS;
			}
		}
		else if (_type == Type::Orthographic)
		{
			res.inv_tan_half_fov = _ortho_size;
			type = CAMERA_TYPE_ORTHO;
		}
		else if (_type == Type::Spherical)
		{
			type = CAMERA_TYPE_SPHERICAL;
			res.inv_tan_half_fov = std::min(2 * std::numbers::pi_v<float>, _fov) * 0.5;
			res.inv_aspect = std::min(std::numbers::pi_v<float>, _fov / _aspect) * 0.5;
		}
		res.flags |= type;
		if (_infinite_far)
		{
			res.flags |= CAMERA_FLAG_INFINITE_FAR;
		}
		if (_reverse_depth)
		{
			res.flags |= CAMERA_FLAG_REVERSE_DEPTH;
		}
		return res;
	}
}