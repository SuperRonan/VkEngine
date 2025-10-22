#include <vkl/Rendering/Light.hpp>

#include <vkl/IO/ImGuiUtils.hpp>

#include <vkl/Maths/View.hpp>

namespace vkl
{
	LightGLSL LightGLSL::MakePoint(vec3 position, vec3 emission)
	{
		LightGLSL res{
			.position = position,
			.flags = LightType::Point,
			.emission = emission,
		};
		return res;
	}

	LightGLSL LightGLSL::MakeDirectional(vec3 dir, vec3 emission)
	{
		LightGLSL res{
			.position = vec3(0, 0, 0),
			.flags = LightType::Directional,
			.emission = emission,
			.direction = dir,
		};
		return res;
	}

	LightGLSL LightGLSL::transform(Matrix3x4f const& mat) const
	{
		LightGLSL res;

		res.flags = flags;
		res.emission = emission;

		res.position = mat * Homogeneous(position);
		res.direction = Normalize(DirectionMatrix(mat3(mat)) * direction);
		
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
			float f = Average(_emission);
			_emission = vec3::Constant(f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Snap to Gray luminance"))
		{
			vec3 w(0.2126, 0.7152, 0.0722);
			float f = Dot(w, _emission);
			_emission = vec3::Constant(f);
		}

		float intensity = Average(_emission);
		float old_intensity = intensity;
		bool changed = ImGui::SliderFloat("Intensity", &intensity, 0, 50, "%.3f", ImGuiSliderFlags_Logarithmic);

		if (changed && old_intensity > 0.0f)
		{
			_emission *= (intensity / old_intensity);
		}
		
		ImGui::ColorEdit3("Emission", _emission.data(), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs);

		static thread_local ImGuiListSelection gui_shadow_bias_mode = ImGuiListSelection::CI{
			.name = "Shadow Bias Mode",
			.mode = ImGuiListSelection::Mode::Combo,
			.same_line = true,
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
			.type = LightType::Point,
			.emission = ci.emission,
			.enable_shadow_map = ci.enable_shadow_map,
		}),
		_position(ci.position),
		_z_near(ci.z_near)
	{}

	LightGLSL PointLight::getAsGLSL(Matrix3x4f const& xform) const
	{
		const vec3 hpos = xform * Homogeneous(_position);
		LightGLSL res = LightGLSL::MakePoint(hpos, _emission);
		res.flags = flags();
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		res.z_near = _z_near;
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
			.type = LightType::Directional,
			.emission = ci.emission,
			.enable_shadow_map = false,
		}),
		_direction(Normalize(ci.direction))
	{}

	uint32_t DirectionalLight::flags()const
	{
		uint32_t res = Light::flags();
		return res;
	}

	LightGLSL DirectionalLight::getAsGLSL(Matrix3x4f const& xform) const
	{
		const vec3 dir = Normalize(DirectionMatrix(mat3(xform)) * _direction);
		LightGLSL res = LightGLSL::MakeDirectional(dir, _emission);
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		return res;
	}

	void DirectionalLight::declareGui(GuiContext& ctx)
	{
		Light::declareGui(ctx);

		ImGui::PushID(name().c_str());

		ImGui::PopID();
	}




	SpotBeamLight::SpotBeamLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = ci.is_beam ? LightType::Beam : LightType::Spot,
			.emission = ci.emission,
			.enable_shadow_map = ci.enable_shadow_map,
		}),
		_position(ci.position),
		_direction(Normalize(ci.direction)),
		_up(Normalize(ci.up)),
		_ratio(ci.aspect_ratio),
		_opening(ci.opening),
		_attenuation(ci.attenuation)
	{
		
	}

	uint32_t SpotBeamLight::flags()const
	{
		uint32_t res = Light::flags();
		res |= ((uint32_t(_attenuation) & 0b11) << 16);
		return res;
	}

	LightGLSL SpotBeamLight::getAsGLSL(Matrix3x4f const& xform)const
	{
		LightGLSL res;
		res.emission = _emission;
		auto NotZero = [](float f){return f != 0.0f ? f : 1;};
		if (_preserve_intensity_from_opening)
		{
			res.emission *= 1.0 / std::max(sqr(NotZero(_opening)) * NotZero(_ratio), std::numeric_limits<float>::epsilon());
		}
		res.flags = SpotBeamLight::flags();
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		const mat3 dir_mat = DirectionMatrix(mat3(xform));
		vec3 h_position = xform * Homogeneous(_position);
		res.position = h_position;
		const vec3 direction = Normalize(dir_mat * _direction);
		const vec3 up = Normalize(dir_mat * _up);
		res.direction = direction;
		if (_type == LightType::Spot)
		{
			res.z_near = _znear;
			res.spot = LightGLSL::SpotSpecific{
				.up = up,
				.tan_half_fov = std::tan(_opening * 0.5f),
				.aspect = _ratio,
			};
		}
		else
		{
			res.beam = LightGLSL::BeamSpecific{
				.up = up,
				.radius = _opening,
				.aspect = _ratio,
			};
		}
		return res;
	}

	void SpotBeamLight::declareGui(GuiContext& ctx)
	{
		Light::declareGui(ctx);
		ImGui::PushID(name().c_str());
		if (_type == LightType::Spot)
		{
			ImGui::SliderAngle("Angle", &_opening, 0, 180);
		}
		else
		{
			ImGui::SliderFloat("Opening", &_opening, 0, 1, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
		}
		ImGui::SliderFloat("Aspect Ratio", &_ratio, 0, 10, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
		ImGui::Checkbox("Preserve total intensity from opening", &_preserve_intensity_from_opening);
		static ImGuiListSelection gui_attenuation(ImGuiListSelection::CI{
			.name = "Attenuation",
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.same_line = true,
			.labels = {"None", "Linear", "Quadratic", "Inside"},
		});
		gui_attenuation.setIndex(_attenuation);
		if (gui_attenuation.declare())
		{
			_attenuation = gui_attenuation.index();
		}

		ImGui::Checkbox("Enable Shadow Map", &_enable_shadow_map);

		if (_type == LightType::Spot)
		{
			ImGui::SliderFloat("Z Near", &_znear, 0, 10, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
		}

		ImGui::PopID();
	}
}