#include "Light.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <Core/IO/ImGuiUtils.hpp>

namespace vkl
{
	LightGLSL LightGLSL::MakePoint(vec3 position, vec3 emission)
	{
		LightGLSL res{
			.position = position,
			.flags = LightType::POINT,
			.emission = emission,
		};
		return res;
	}

	LightGLSL LightGLSL::MakeDirectional(vec3 dir, vec3 emission)
	{
		LightGLSL res{
			.position = vec3(0, 0, 0),
			.flags = LightType::DIRECTIONAL,
			.emission = emission,
			.direction = dir,
		};
		return res;
	}

	LightGLSL LightGLSL::transform(mat4 const& mat) const
	{
		LightGLSL res;

		res.flags = flags;
		res.emission = emission;

		vec4 hpos = (mat * vec4(position, 1));
		res.position = hpos;
		res.direction = glm::normalize(directionMatrix(mat) * direction);
		
		return res;
	}



	Light::Light(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type),
		_emission(ci.emission),
		_enable_shadow_map(ci.enable_shadow_map)
	{}

	uint32_t Light::flags() const
	{
		uint32_t res = _type;
		if (_enable_shadow_map)
		{
			res |= (1 << shadowMapBitIndex());
		}
		res |= uint32_t(_shadow_bias_mode) << (shadowMapBitIndex() + 1);
		if (_shadow_bias_include_cos_theta)
		{
			res |= (1 << (shadowMapBitIndex() + 3));
		}
		return res;
	}

	void Light::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(this);

		ImGui::Text("Name: ");
		ImGui::SameLine();
		ImGui::Text(name().c_str());

		if (ImGui::Button("Snap to Gray average"))
		{
			float f = (_emission.x + _emission.y + _emission.z) / 3;
			_emission = vec3(f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Snap to Gray luminance"))
		{
			vec3 w(0.2126, 0.7152, 0.0722);
			float f = glm::dot(w, _emission);
			_emission = vec3(f);
		}

		float intensity = (_emission.x + _emission.y + _emission.z) / 3;
		float old_intensity = intensity;
		bool changed = ImGui::SliderFloat("Intensity", &intensity, 0, 10);

		if (changed && old_intensity > 0.0f)
		{
			_emission *= (intensity / old_intensity);
		}
		
		ImGui::ColorPicker3("Emission", &_emission.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

		static thread_local ImGuiListSelection gui_shadow_bias_mode = ImGuiListSelection::CI{
			.name = "Shadow Bias Mode",
			.mode = ImGuiListSelection::Mode::Combo,
			.options = {
				ImGuiListSelection::Option{
					.name = "None",
					.desc = "No shadow bias",
				},
				ImGuiListSelection::Option{
					.name = "Float bit Offset",
				},
				ImGuiListSelection::Option{
					.name = "Float Multiplication"
				},
				ImGuiListSelection::Option{
					.name = "Float Addition",
				},
			},
			.same_line = true,
		};
		gui_shadow_bias_mode.setIndex(size_t(_shadow_bias_mode));
		if (gui_shadow_bias_mode.declare())
		{
			const ShadowBiasMode new_mode = ShadowBiasMode(gui_shadow_bias_mode.index());
			_shadow_bias_mode = new_mode;
			switch (_shadow_bias_mode)
			{
			case ShadowBiasMode::None:
			case ShadowBiasMode::Offset:
			case ShadowBiasMode::FloatAdd:
				_int_shadow_bias = 0;
			break;
			case ShadowBiasMode::FloatMult:
				_float_shadow_bias = float(1);
			break;
			}
		}
		if (_shadow_bias_mode == ShadowBiasMode::Offset)
		{
			ImGui::InputInt("Offset", &_int_shadow_bias);
		}
		else if (_shadow_bias_mode == ShadowBiasMode::FloatMult)
		{
			float omm = 1 - _float_shadow_bias;
			if (ImGui::SliderFloat("Multiplicator", &omm, 0, 1, "1 - %.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
			{
				_float_shadow_bias = 1 - omm;
			}
			ImGui::BeginDisabled();
			ImGui::InputFloat("Mult:", &_float_shadow_bias, 0, 0, "%.5f");
			ImGui::EndDisabled();
		}
		else if (_shadow_bias_mode == ShadowBiasMode::FloatMult)
		{
			ImGui::SliderFloat("Bias", &_float_shadow_bias, -1, 1, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
		}

		ImGui::Checkbox("Shadow Bias include cos theta", &_shadow_bias_include_cos_theta);

		ImGui::PopID();
	}


	PointLight::PointLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::POINT,
			.emission = ci.emission,
			.enable_shadow_map = ci.enable_shadow_map,
		}),
		_position(ci.position),
		_z_near(ci.z_near)
	{}

	LightGLSL PointLight::getAsGLSL(mat4 const& xform) const
	{
		const vec4 hpos = xform * vec4(_position, 1);
		LightGLSL res = LightGLSL::MakePoint(vec3(hpos), _emission);
		res.flags = flags();
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		res.matrix[0][0] = _z_near;
		return res;
	}

	uint32_t PointLight::flags() const
	{
		uint32_t res = Light::flags();
		return res;
	}

	void PointLight::declareGui(GuiContext& ctx)
	{
		Light::declareGui(ctx);

		ImGui::PushID(name().c_str());

		ImGui::Checkbox("Enable Shadow Map", &_enable_shadow_map);

		ImGui::SliderFloat("Z Near", &_z_near, 0, 10, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);

		ImGui::PopID();
	}



	DirectionalLight::DirectionalLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::POINT,
			.emission = ci.emission,
			.enable_shadow_map = false,
		}),
		_direction(glm::normalize(ci.direction))
	{}

	uint32_t DirectionalLight::flags()const
	{
		uint32_t res = Light::flags();
		return res;
	}

	LightGLSL DirectionalLight::getAsGLSL(mat4 const& xform) const
	{
		const vec3 dir = glm::normalize(directionMatrix(xform) * _direction);
		LightGLSL res = LightGLSL::MakeDirectional(dir, _emission);
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		res.matrix[0][0];
		return res;
	}

	void DirectionalLight::declareGui(GuiContext& ctx)
	{
		Light::declareGui(ctx);

		ImGui::PushID(name().c_str());

		ImGui::PopID();
	}




	SpotLight::SpotLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::SPOT,
			.emission = ci.emission,
			.enable_shadow_map = ci.enable_shadow_map,
		}),
		_position(ci.position),
		_direction(glm::normalize(ci.direction)),
		_up(glm::normalize(ci.up)),
		_ratio(ci.aspect_ratio),
		_fov(ci.fov),
		_attenuation(ci.attenuation)
	{
		
	}

	uint32_t SpotLight::flags()const
	{
		uint32_t res = Light::flags();
		res |= ((uint32_t(_attenuation) & 0b11) << 16);
		return res;
	}

	LightGLSL SpotLight::getAsGLSL(mat4 const& xform)const
	{
		LightGLSL res;
		res.emission = _emission;
		if (_preserve_intensity_from_fov)
		{
			res.emission *= 1.0 / std::max(_fov, std::numeric_limits<float>::epsilon());
		}
		res.flags = SpotLight::flags();
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		const mat3 dir_mat = directionMatrix(xform);
		res.position = vec3(xform * vec4(_position, 1));
		const vec3 direction = glm::normalize(dir_mat * _direction);
		const vec3 up = glm::normalize(dir_mat * _up);
		const mat4 look_at = glm::lookAt(res.position, res.position + direction, -up);
		const mat4 proj = glm::infinitePerspective<float>(_fov, _ratio, _znear);
		//const mat4 proj = glm::perspective<float>(_fov, _ratio, _znear, 2 * _znear);
		res.direction = direction;
		res.matrix = (proj * look_at);
		return res;
	}

	void SpotLight::declareGui(GuiContext& ctx)
	{
		Light::declareGui(ctx);
		ImGui::PushID(name().c_str());
		ImGui::SliderAngle("Angle", &_fov, 0, 180);
		ImGui::Checkbox("Preserve total intensity from angle", &_preserve_intensity_from_fov);
		static ImGuiListSelection gui_attenuation(ImGuiListSelection::CI{
			.name = "Attenuation",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.labels = {"None", "Linear", "Quadratic", "Root"},
			.same_line = true,
		});
		gui_attenuation.setIndex(_attenuation);
		if (gui_attenuation.declare())
		{
			_attenuation = gui_attenuation.index();
		}

		ImGui::Checkbox("Enable Shadow Map", &_enable_shadow_map);

		ImGui::PopID();
	}
}