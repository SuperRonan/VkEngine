#include <vkl/Rendering/Camera.hpp>
#include <vkl/Maths/Transforms.hpp>

#include <ShaderLib/Rendering/Camera/CameraDefinitions.h>

namespace vkl
{
	Camera::Camera(CreateInfo const& ci) :
		_resolution_ifp(ci.resolution),
		_near(ci.znear),
		_far(ci.zfar)
	{
		_gui_type = ImGuiListSelection::CI{
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
			if (_infinite_perspective || std::isinf(_far))
			{
				res = InfinitePerspectiveProjFromFOV(_fov, _aspect, _near);
			}
			else
			{
				res = PerspectiveProjFromFOV(_fov, _aspect, vec2(_near, _far));
			}
		}
		else if (_type == Type::Orthographic)
		{
			const AABB3f aabb = getOrthoAABB();
			res = OrthoProj(aabb.bottom(), aabb.top());
		}
		return res;
	}

	Camera::mat4 Camera::getProjToCam() const
	{
		mat4 res;
		if (_type == Type::Perspective)
		{
			if (_infinite_perspective || std::isinf(_far))
			{
				res = InverseInfinitePerspectiveProjFromFOV(_fov, _aspect, _near);
			}
			else
			{
				res = InversePerspectiveProjFromFOV(_fov, _aspect, vec2(_near, _far));
			}
		}
		else if (_type == Type::Orthographic)
		{
			const AABB3f aabb = getOrthoAABB();
			res = InverseOrthoProj(aabb.bottom(), aabb.top());
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

	void Camera::declareGui(GuiContext& ctx)
	{
		if (ImGui::CollapsingHeader("Camera"))
		{
			ImGui::Checkbox("Infinite perspective", &_infinite_perspective);
			float f;
			f = _near;
			if (ImGui::SliderFloat("near plane", &f, 0, _far))
			{
				_near = f;
			}
			
			if (_type != Type::Spherical)
			{
				f = _far;
				if (ImGui::SliderFloat("far plane", &f, _near, 1e4 * _near))
				{
					_far = f;
				}
			}

			if (_gui_type.declare())
			{
				_type = Type(_gui_type.index());
			}

			if (_type == Type::Perspective || _type == Type::Spherical)
			{
				f = _fov;
				float max_fov = 180;
				if(_type == Camera::Type::Spherical)	max_fov *= 2;
				if (ImGui::SliderAngle("FOV", &f, 0, max_fov))
				{
					_fov = f;
				}
			}

			if (_type == Type::Perspective)
			{
				ImGui::SliderFloat("Aperture", &_aperture, 0, 100, "%.1f mm", ImGuiSliderFlags_NoRoundToFormat);
				ImGui::SliderFloat("Focal distance", &_focal_distance, 0, 100, "%.3f m", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_Logarithmic);
				
				float f = focalLength() * 1e3f;
				if (ImGui::InputFloat("Focal Length", &f, 0, 0, "%.1f mm", ImGuiInputTextFlags_EnterReturnsTrue))
				{
					// Auto focus
					float fm = f / 1e3f;
					_focal_distance = fm * distanceFilmLens() * rcp(abs(fm - distanceFilmLens()));
				}
				ImGui::Text("f-number: f / %.1f", fNumber());
			}
			else if (_type == Type::Orthographic)
			{

			}
		}
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
			res.inv_tan_half_fov = std::min(2 * std::numbers::pi_v<float>, _fov);
			res.inv_aspect = std::min(std::numbers::pi_v<float>, _fov / _aspect);
		}
		res.flags |= type;
		return res;
	}

	FirstPersonCameraController::FirstPersonCameraController(CreateInfo const& ci) :
		CameraController(ci.camera),
		_keyboard(ci.keyboard),
		_mouse(ci.mouse),
		_gamepad(ci.gamepad)
	{}

	void FirstPersonCameraController::updateCamera(float dt, MouseEventListener* mouse, KeyboardStateListener* keyboard, GamepadListener* gamepad)
	{
		Camera::CameraDelta delta;

		const float fov_sensitivity = (_camera->fov()) / (std::numbers::pi / 2.0);

		if(!mouse) mouse = _mouse;
		if(!keyboard) keyboard = _keyboard;
		if(!gamepad) gamepad = _gamepad;

		if (keyboard)
		{

			if (keyboard->getKey(_key_forward).currentlyPressed())
			{
				delta.movement[2] += 1;
			}
			if (keyboard->getKey(_key_backward).currentlyPressed())
			{
				delta.movement[2] += -1;
			}
			if (keyboard->getKey(_key_left).currentlyPressed())
			{
				delta.movement[0] += -1;
			}
			if (keyboard->getKey(_key_right).currentlyPressed())
			{
				delta.movement[0] += 1;
			}
			if (keyboard->getKey(_key_upward).currentlyPressed())
			{
				delta.movement[1] += 1;
			}
			if (keyboard->getKey(_key_downward).currentlyPressed())
			{
				delta.movement[1] += -1;
			}

			delta.movement = SafeNormalize(delta.movement);
		}



		if (mouse)
		{
			if (mouse->getButton(SDL_BUTTON_LEFT).currentlyPressed())
			{
				delta.angle += mouse->getPos().delta() * _mouse_sensitivity;
			}
			
			delta.fov *= std::exp2(-mouse->getScroll().current[1] * _fov_sensitivity);
		}

		if (gamepad)
		{
			delta.movement[0] += gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTX).current;
			delta.movement[2] -= gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTY).current;

			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_A).currentlyPressed())
			{
				delta.movement[1] += 1;
			}
			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_B).currentlyPressed())
			{
				delta.movement[1] -= 1;
			}

			delta.angle[0] += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTX).current * dt * _joystick_sensitivity;
			delta.angle[1] += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTY).current * dt * _joystick_sensitivity;

			float fov_zoom = gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER).current - gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER).current;
			delta.fov *= exp(fov_zoom * _fov_sensitivity * dt * 1e1);
		}

		delta.movement *= dt * _movement_speed;
		delta.angle *= fov_sensitivity;
		_camera->update(delta);
	}
}