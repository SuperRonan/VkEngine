#include <vkl/Rendering/Camera.hpp>
#include <vkl/Maths/Transforms.hpp>

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
				//"ReversedPerspective"
			},
			.default_index = 0,
		};
	}

	Ray Camera::getRay(vec2 uv) const
	{
		const vec2 cp = uvToClipSpace(uv);
		return Ray{
			.origin = _position,
			.direction = glm::normalize(_direction + cp.x * _right * _aspect - cp.y * _up),
		};
	}

	mat4 Camera::getCamToProj()const
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
		else if (_type == Type::ReversedPerspective)
		{
			mat4 p = glm::perspective(_fov, _aspect, _near, _far);
			mat4 r = glm::rotate(mat4(1), glm::radians(180.0f), _up);
			mat4 t = p;
			res = p * r * t;
		}
		return res;
	}

	mat4 Camera::getProjToCam() const
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

	mat4 Camera::getWorldToProj() const
	{
		return getCamToProj() * mat4(getWorldToCam());
	}

	mat4 Camera::GetProjToWorld() const
	{
		return getCamToWorld() * getProjToCam();
	}

	void Camera::update(CameraDelta const& delta)
	{
		const vec3 front = glm::cross(_up, _right);
		const float d = glm::dot(front, _direction);
		vec3 dp = delta.movement.x * _right + delta.movement.y * _up + delta.movement.z * front;

		dp = normalizeSafe(dp) * glm::length(delta.movement);
		_position += dp;

		_direction = rotate(_direction, _up, -delta.angle.x);
		computeInternal();

		const float y_angle = [&]() {
			const float current_cos_angle = glm::dot(_direction, _up);
			const float current_angle = std::acos(current_cos_angle);
			const float margin = 0.01;
			const float new_angle = std::clamp<float>(current_angle + delta.angle.y, margin * std::numbers::pi, (1.0f - margin) * std::numbers::pi);
			const float diff = new_angle - current_angle;
			//std::cout << glm::degrees(new_angle) << ", " << glm::degrees(diff) << ", " << glm::degrees(_fov) << std::endl;
			return diff;
		}();
		_direction = rotate(_direction, _right, -y_angle);
		//computeInternal();

		if (_type == Type::Perspective)
		{
			_fov *= delta.fov;
			_fov = std::clamp(_fov, glm::radians(1e-1f), glm::radians(179.0f));
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
			
			f = _far;
			if (ImGui::SliderFloat("far plane", &f, _near, 1e4 * _near))
			{
				_far = f;
			}

			if (_gui_type.declare())
			{
				_type = Type(_gui_type.index());
			}

			if (_type == Type::Perspective)
			{
				f = _fov;
				if (ImGui::SliderAngle("FOV", &f, 0, 180))
				{
					_fov = f;
				}

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
		res.flags |= static_cast<uint32_t>(_type);
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
				delta.movement.z += 1;
			}
			if (keyboard->getKey(_key_backward).currentlyPressed())
			{
				delta.movement.z += -1;
			}
			if (keyboard->getKey(_key_left).currentlyPressed())
			{
				delta.movement.x += -1;
			}
			if (keyboard->getKey(_key_right).currentlyPressed())
			{
				delta.movement.x += 1;
			}
			if (keyboard->getKey(_key_upward).currentlyPressed())
			{
				delta.movement.y += 1;
			}
			if (keyboard->getKey(_key_downward).currentlyPressed())
			{
				delta.movement.y += -1;
			}

			delta.movement = normalizeSafe(delta.movement);
		}



		if (mouse)
		{
			if (mouse->getButton(SDL_BUTTON_LEFT).currentlyPressed())
			{
				delta.angle += mouse->getPos().delta() * _mouse_sensitivity;
			}
			
			delta.fov *= exp2(-mouse->getScroll().current.y * _fov_sensitivity);
		}

		if (gamepad)
		{
			delta.movement.x += gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTX).current;
			delta.movement.z -= gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFTY).current;

			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_A).currentlyPressed())
			{
				delta.movement.y += 1;
			}
			if (gamepad->getButton(SDL_GAMEPAD_BUTTON_LABEL_B).currentlyPressed())
			{
				delta.movement.y -= 1;
			}

			delta.angle.x += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTX).current * dt * _joystick_sensitivity;
			delta.angle.y += gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHTY).current * dt * _joystick_sensitivity;

			float fov_zoom = gamepad->getAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER).current - gamepad->getAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER).current;
			delta.fov *= exp(fov_zoom * _fov_sensitivity * dt * 1e1);
		}

		delta.movement *= dt * _movement_speed;
		delta.angle *= fov_sensitivity;
		_camera->update(delta);
	}
}