#include <vkl/Rendering/GammaCorrection.hpp>
#include <imgui/imgui.h>
#include <vkl/Execution/SamplerLibrary.hpp>
#include <vkl/VkObjects/DetailedVkFormat.hpp>
#include <vkl/IO/ImGuiUtils.hpp>

namespace vkl
{
	GammaCorrection::GammaCorrection(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_sampler(ci.sampler),
		_sets_layouts(ci.sets_layouts),
		_target_window(ci.target_window)
	{
		if (!_src)
		{
			_src = _dst;
		}
		createInternalResources();
	}

	void GammaCorrection::createInternalResources()
	{
		const bool use_separate_src = _src != _dst;
		const std::filesystem::path folder = application()->mountingPoints()["ShaderLib"] + "/ToneMap/";

		ShaderBindings bindings;
		bindings.push_back(Binding{
			.image = _dst,
			.binding = 0,
		});

		if (use_separate_src)
		{
			if (!_sampler)
			{
				_sampler = application()->getSamplerLibrary().getSampler(SamplerLibrary::SamplerInfo{
					
				});
			}
			bindings.push_back(Binding{
				.image = _src,
				.sampler = _sampler,
				.binding = 1,
			});
		}
		
		Dyn<DefinitionsList> definitions = [this](DefinitionsList & res)
		{
			res.clear();
			const char d = (_src == _dst) ? '0' : '1';
			res.pushBackFormatted("USE_SEPARATE_SOURCE {:c}"sv, d);
			res.pushBackFormatted("DST_FORMAT {:s}", _dst_glsl_format);
			res.pushBackFormatted("MODE {}", static_cast<uint32_t>(_mode));
			return res;
		};

		_compute_tonemap = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".shader",
			.shader_path = folder / "GammaCorrection.comp",
			.sets_layouts = _sets_layouts,
			.bindings = bindings,
			.definitions = definitions,
		});
	}

	void GammaCorrection::updateResources(UpdateContext& ctx)
	{
		if (_auto_fit_to_window && _target_window)
		{
			SwapchainInfo info{
				.format = _target_window->swapchain()->instance()->format(),
			};

			if (info != _prev_swapchain_info)
			{
				ColorCorrectionInfo info = _target_window->getColorCorrectionInfo();
				_mode = info.mode;
				switch (_mode)
				{
					case ColorCorrectionMode::Gamma:
						_gamma_params = info.params.gamma;
					break;
					case ColorCorrectionMode::HLG:
						_hlg_params = info.params.hlg;
					break;
				}
			}

			_prev_swapchain_info = info;
		}
		const DetailedVkFormat dst_format = DetailedVkFormat::Find(_dst->format().value());
		_dst_glsl_format = dst_format.getGLSLName();

		ctx.resourcesToUpdateLater() += _compute_tonemap;
	}

	void GammaCorrection::execute(ExecutionRecorder& exec)
	{
		bool enable = false;
		enable |= (_mode != ColorCorrectionMode::PassThrough);
		enable |= (_src != _dst);
		if (enable)
		{
			struct PC
			{
				union
				{
					float exposure;
					float ref_white;
				};
				float gamma;
			};
			PC pc;
			if (_mode == ColorCorrectionMode::Gamma)
			{
				pc.exposure = _gamma_params.exposure;
				pc.gamma = _gamma_params.gamma;
			}
			else if (_mode == ColorCorrectionMode::HLG)
			{
				pc.ref_white = _hlg_params.white_point;
			}
			exec(_compute_tonemap->with(ComputeCommand::SingleDispatchInfo{
				.extent = _dst->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
		}
	}

	float GammaCorrection::computeTransferFunction(float linear) const
	{
		float res;
		switch (_mode)
		{
			case ColorCorrectionMode::PassThrough:
				res = linear;
			break;
			case ColorCorrectionMode::Gamma:
				res = std::pow(linear * _gamma_params.exposure, _gamma_params.gamma);
			break;
			case ColorCorrectionMode::HLG:
			{
				linear *= _hlg_params.white_point;
				const float a = 0.17883277;
				const float b = 1.0 - 4.0 * a;
				const float c = 0.5 - a * std::log(4.0 * a);
				if (linear <= 1)
				{
					res = 0.5 * std::sqrt(linear);
				}
				else
				{
					res = a * std::log(linear - b) + c;
				}
			}
			break;
		}
		return res;
	}

	void GammaCorrection::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			ImGui::PushID(name().c_str());
	
			ImGui::Checkbox("Auto Fit to window: ", &_auto_fit_to_window);

			static thread_local ImGuiListSelection gui_mode = ImGuiListSelection::CI{
				.name = "Mode",
				.mode = ImGuiListSelection::Mode::RadioButtons,
				.same_line = true,
				.options = {
					ImGuiListSelection::Option{
						.name = "None",
					},
					ImGuiListSelection::Option{
						.name = "Gamma",
					},
					ImGuiListSelection::Option{
						.name = "HLG",
						.desc = "Hybrid Log Gamma",
					},
				},
			};

			bool changed = false;
			{
				size_t mode_index = static_cast<size_t>(_mode);
				if (gui_mode.declare(mode_index))
				{
					_mode = static_cast<ColorCorrectionMode>(mode_index);
					changed = true;
				}
			}


			if (_mode == ColorCorrectionMode::Gamma)
			{
				float log_exposure = std::log2(_gamma_params.exposure);
				bool exposure_changed = false;
				exposure_changed = ImGui::SliderFloat("log2(Exposure)", &log_exposure, -5, 5, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
				ImGui::SameLine();
				if (ImGui::Button("-"))
				{
					exposure_changed |= true;
					log_exposure -= 0.5;
				}
				ImGui::SameLine();
				if (ImGui::Button("+"))
				{
					exposure_changed |= true;
					log_exposure += 0.5;
				}
				if (exposure_changed)
				{
					changed = true;
					_gamma_params.exposure = std::exp2f(log_exposure);
				}
				ImGui::Text("Exposure: %f", _gamma_params.exposure);
				//ImGui::Text("Snap to: ");
				//ImGui::SameLine();
			
				ImGui::PushID('G');
				changed |= ImGui::SliderFloat("Gamma", &_gamma_params.gamma, 0.1, 4);
				ImGui::PopID();
				ImGui::Text("Snap Gamma: ");
				std::array<float, 4> snap_values = {1.0, 1.2, 2.0, 2.2};
				char str_buffer[16];
				for (size_t i = 0; i < snap_values.size(); ++i)
				{
					ImGui::SameLine();
					*std::format_to(str_buffer, "{:1}", snap_values[i]) = char(0);
					bool snap = ImGui::Button(str_buffer);
					if (snap)
					{
						_gamma_params.gamma = snap_values[i];
						changed |= true;
					}
				}
				ImGui::Separator();

			}
			else if (_mode == ColorCorrectionMode::HLG)
			{
				changed = ImGui::SliderFloat("White Point", &_hlg_params.white_point, 0, 1, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			}


			if (_plot_raw_radiance.size() != _plot_samples)
			{
				changed = true;
				_plot_raw_radiance.resize(_plot_samples);
				_plot_corrected_radiance.resize(_plot_samples);
			}

			if (changed)
			{
				for (size_t i = 0; i < _plot_samples; ++i)
				{
					float t = (float(i) + (float(i) / float(_plot_samples - 1))) / float(_plot_samples);
					_plot_raw_radiance[i] = std::lerp(_plot_min_radiance, _plot_max_radiance, t);
					_plot_corrected_radiance[i] = computeTransferFunction(_plot_raw_radiance[i]);
				}

			}

			ImGui::PlotLines("Gamma Correction Preview", _plot_corrected_radiance.data(), _plot_samples, 0, nullptr, 0, _plot_corrected_radiance.back(), ImVec2(0, 200));
			
			ImGui::PopID();
		}
	}
}