#include <vkl/Rendering/Light.hpp>

#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>

#include <vkl/Maths/View.hpp>

#include <ShaderLib/Rendering/Lights/Definitions.h>

#include <vkl/Rendering/SpectrumWaveLength.hpp>

//#include <that/stl_ext/alignment.hpp>

namespace vkl
{
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
		_emission_options(ci.emission_options),
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
		if (_emission_options & EMISSION_FLAG_BLACK_BODY_BIT)
		{
			res |= LIGHT_BLACK_BODY_EMISSION_BIT_FLAG;
		}
		return res;
	}

	namespace GUI
	{
		class LightInspector : public Panel
		{
		protected:

			std::shared_ptr<Light> _target;

			ImGuiListSelection _shadow_bias_mode;

		public:

			LightInspector(std::shared_ptr<Light> const& target):
				Panel(_target->application(), std::format("{} - Light Inspector##{}", target->name(), reinterpret_cast<uintptr_t>(target.get()))),
				_target(target)
			{
				_shadow_bias_mode = ImGuiListSelection::CI{
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
			}

			virtual void declareInline(Context& ctx) override
			{
				Light::DeclareEmission(_target->_emission, _target->_emission_options);

				_shadow_bias_mode.setIndex(size_t(_target->_shadow_bias_mode));
				if (_shadow_bias_mode.declare())
				{
					const Light::ShadowBiasMode new_mode = Light::ShadowBiasMode(_shadow_bias_mode.index());
					_target->_shadow_bias_mode = new_mode;
					switch (_target->_shadow_bias_mode)
					{
					case Light::ShadowBiasMode::None:
					case Light::ShadowBiasMode::Offset:
					case Light::ShadowBiasMode::FloatAdd:
						_target->_int_shadow_bias = 0;
						break;
					case Light::ShadowBiasMode::FloatMult:
						_target->_float_shadow_bias = float(1);
						break;
					}
				}
				if (_target->_shadow_bias_mode == Light::ShadowBiasMode::Offset)
				{
					ImGui::InputInt("Offset", &_target->_int_shadow_bias);
				}
				else if (_target->_shadow_bias_mode == Light::ShadowBiasMode::FloatMult)
				{
					float omm = 1 - _target->_float_shadow_bias;
					if (ImGui::SliderFloat("Multiplicator", &omm, 0, 1, "1 - %.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
					{
						_target->_float_shadow_bias = 1 - omm;
					}
					ImGui::BeginDisabled();
					ImGui::InputFloat("Mult:", &_target->_float_shadow_bias, 0, 0, "%.5f");
					ImGui::EndDisabled();
				}
				else if (_target->_shadow_bias_mode == Light::ShadowBiasMode::FloatMult)
				{
					ImGui::SliderFloat("Bias", &_target->_float_shadow_bias, -1, 1, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				}

				ImGui::Checkbox("Shadow Bias include cos theta", &_target->_shadow_bias_include_cos_theta);
			}
		};
	}

	bool Light::DeclareEmission(vec3& emission, uint8_t& options)
	{
		bool res = false;
		bool black_body = (options & EMISSION_FLAG_BLACK_BODY_BIT);
		if (ImGui::Checkbox("Black Body Emission", &black_body))
		{
			if (black_body)
			{
				options = BlackBodyEmission(1);
				emission.x() = 5500;
				emission.y() = 1;
			}
			else
			{
				options = 0;
				emission = vec3::Ones();
			}
			res |= true;
		}
		if (black_body)
		{
			res |= ImGui::SliderFloat("Temperature", &emission.x(), 1e3, 1e4, "%.0fK", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PushID(1);
			res |= ImGui::SliderFloat("Intensity", &emission.y(), 1e-2, 1e2, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopID();

			size_t norm = ExtractBlackBodyEmissionNorm(options);
			static thread_local ImGuiListSelection normalizations = ImGuiListSelection::CI{
				.name = "Normalization",
				.mode = ImGuiListSelection::Mode::RadioButtons,
				.same_line = true,
				.options = {
					ImGuiListSelection::Option{
						.name = "None",
					},
					ImGuiListSelection::Option{
						.name = "Visible",
					},
					ImGuiListSelection::Option{
						.name = "Total",
					},
				},
			};
			if (normalizations.declare(norm))
			{
				options = BlackBodyEmission(uint(norm));
			}
		}
		else
		{
			if (ImGui::Button("Snap to Gray average"))
			{
				float f = Average(emission);
				emission = vec3::Constant(f);
				res |= true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Snap to Gray luminance"))
			{
				vec3 w(0.2126, 0.7152, 0.0722);
				float f = Dot(w, emission);
				emission = vec3::Constant(f);
				res |= true;
			}
			float intensity = Average(emission);
			float old_intensity = intensity;
			ImGui::PushID(0);
			bool changed = ImGui::SliderFloat("Intensity", &intensity, 0, 50, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopID();

			if (changed && old_intensity > 0.0f)
			{
				emission *= (intensity / old_intensity);
				res |= true;
			}

			res |= ImGui::ColorEdit3("Emission", emission.data(), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs);
		}
		return res;
	}

	vec3 Light::NormalizeEmission(vec3 emission, uint8_t options, float norm)
	{
		vec3 res = emission;
		if (options & EMISSION_FLAG_BLACK_BODY_BIT)
		{
			res.y() /= (norm * vkl::EvalBlackBodySpectralRadianceNorm(res.x(), ExtractBlackBodyEmissionNorm(options)));
		}
		else
		{
			res /= norm;
		}
		return res;
	}

	PointLight::PointLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::Point,
			.emission = ci.emission,
			.emission_options = ci.emission_options,
			.enable_shadow_map = ci.enable_shadow_map,
		}),
		_position(ci.position),
		_z_near(ci.z_near)
	{}

	LightGLSL PointLight::getAsGLSL(Matrix3x4f const& xform) const
	{
		const vec3 hpos = xform * Homogeneous(_position);
		LightGLSL res = {};
		res.position = hpos;
		res.emission = NormalizeEmission(_emission, _emission_options);
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

	namespace GUI
	{
		template <std::strictly_derived_from<Light> L>
		class DerivedLightInspector : public LightInspector
		{
		protected:

		public:

			DerivedLightInspector(std::shared_ptr<L> const& target, std::string_view class_name={}) :
				LightInspector(target)
			{
				if (!class_name.empty())
				{
					_name.clear();
					std::format_to(std::back_inserter(_name), "{} - {} Inspector##{}", _target->name(), class_name, reinterpret_cast<uintptr_t>(_target.get()));
				}
			}
		
			L* target() const
			{
				return static_cast<L*>(_target.get());
			}
		};

		class PointLightInspector : public DerivedLightInspector<PointLight>
		{
		protected:

			using Parent = DerivedLightInspector<PointLight>;

		public:

			PointLightInspector(std::shared_ptr<PointLight> const& target) :
				Parent(target, "Point Light")
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				LightInspector::declareInline(ctx);
				ImGui::Separator();
				ImGui::Checkbox("Enable Shadow Map", &target()->_enable_shadow_map);

				ImGui::SliderFloat("Z Near", &target()->_z_near, 0, 10, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			}
		};
	}

	std::shared_ptr<GUI::Panel> PointLight::makeInspector(std::shared_ptr<Light> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<GUI::PointLightInspector>(std::static_pointer_cast<PointLight>(shared_this));
	}

	DirectionalLight::DirectionalLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = LightType::Directional,
			.emission = ci.emission,
			.emission_options = ci.emission_options,
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
		LightGLSL res = {};
		res.direction = dir;
		res.emission = NormalizeEmission(_emission, _emission_options);
		res.flags = flags();
		res.shadow_bias_data = static_cast<uint32_t>(_int_shadow_bias);
		return res;
	}

	namespace GUI
	{
		class DirectionalLightInspector : public DerivedLightInspector<DirectionalLight>
		{
		protected:

			using Parent = DerivedLightInspector<DirectionalLight>;

		public:

			DirectionalLightInspector(std::shared_ptr<DirectionalLight> const& target) :
				Parent(target, "Directional Light")
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				LightInspector::declareInline(ctx);
			}
		};
	}

	std::shared_ptr<GUI::Panel> DirectionalLight::makeInspector(std::shared_ptr<Light> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<GUI::DirectionalLightInspector>(std::static_pointer_cast<DirectionalLight>(shared_this));
	}




	SpotBeamLight::SpotBeamLight(CreateInfo const& ci):
		Light(Light::CI{
			.app = ci.app,
			.name = ci.name,
			.type = ci.is_beam ? LightType::Beam : LightType::Spot,
			.emission = ci.emission,
			.emission_options = ci.emission_options,
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
		LightGLSL res = {};
		float norm = 1;
		if (_preserve_intensity_from_opening)
		{
			auto NotZero = [](float f){return f != 0.0f ? f : 1;};
			norm = std::max(sqr(NotZero(_opening)) * NotZero(_ratio), std::numeric_limits<float>::epsilon());
		}
		res.emission = NormalizeEmission(_emission, _emission_options, norm);
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

	namespace GUI
	{
		class SpotBeamLightInspector : public DerivedLightInspector<SpotBeamLight>
		{
		protected:

			using Parent = DerivedLightInspector<SpotBeamLight>;

		public:

			SpotBeamLightInspector(std::shared_ptr<SpotBeamLight> const& target) :
				Parent(target, "Spot / Beam Light")
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				LightInspector::declareInline(ctx);
				ImGui::Separator();
				if (target()->_type == LightType::Spot)
				{
					ImGui::SliderAngle("Angle", &target()->_opening, 0, 180);
				}
				else
				{
					ImGui::SliderFloat("Opening", &target()->_opening, 0, 1, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				}
				ImGui::SliderFloat("Aspect Ratio", &target()->_ratio, 0, 10, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				ImGui::Checkbox("Preserve total intensity from opening", &target()->_preserve_intensity_from_opening);
				static ImGuiListSelection gui_attenuation(ImGuiListSelection::CI{
					.name = "Attenuation",
					.mode = ImGuiListSelection::Mode::RadioButtons,
					.same_line = true,
					.labels = {"None", "Linear", "Quadratic", "Inside"},
					});
				gui_attenuation.setIndex(target()->_attenuation);
				if (gui_attenuation.declare())
				{
					target()->_attenuation = gui_attenuation.index();
				}

				ImGui::Checkbox("Enable Shadow Map", &target()->_enable_shadow_map);

				if (target()->_type == LightType::Spot)
				{
					ImGui::SliderFloat("Z Near", &target()->_znear, 0, 10, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				}
			}
		};
	}

	std::shared_ptr<GUI::Panel> SpotBeamLight::makeInspector(std::shared_ptr<Light> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<GUI::SpotBeamLightInspector>(std::static_pointer_cast<SpotBeamLight>(shared_this));
	}
}