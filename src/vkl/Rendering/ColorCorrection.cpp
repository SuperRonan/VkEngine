#include <vkl/Rendering/ColorCorrection.hpp>
#include <imgui/imgui.h>
#include <vkl/Execution/SamplerLibrary.hpp>
#include <vkl/VkObjects/DetailedVkFormat.hpp>
#include <vkl/IO/ImGuiUtils.hpp>

namespace vkl
{
	ColorCorrection::ColorCorrection(CreateInfo const& ci):
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

	void ColorCorrection::createInternalResources()
	{
		const bool use_separate_src = _src != _dst;
		const std::filesystem::path folder = "ShaderLib:/ToneMap/";

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
			.shader_path = folder / "ColorCorrection.comp",
			.sets_layouts = _sets_layouts,
			.bindings = bindings,
			.definitions = definitions,
		});


		_render_test_card = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".RenderTestCard",
			.shader_path = folder / "RenderTestCard.comp",
			.extent = _dst->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = bindings,
			.definitions = definitions,
		});
	}

	void ColorCorrection::updateResources(UpdateContext& ctx)
	{
		if (_show_test_card)
		{
			ctx.resourcesToUpdateLater() += _render_test_card;
		}
		if (_auto_fit_to_window && _target_window)
		{
			SwapchainInfo info{
				.format = _target_window->swapchain()->instance()->format(),
			};

			if (info != _prev_swapchain_info)
			{
				ColorCorrectionInfo info = _target_window->getColorCorrectionInfo();
				_mode = info.mode;
				if (_mode == ColorCorrectionMode::Gamma)
				{
					_gamma = info.params.gamma;
				}
			}

			_prev_swapchain_info = info;
		}
		const DetailedVkFormat dst_format = DetailedVkFormat::Find(_dst->format().value());
		_dst_glsl_format = dst_format.getGLSLName();

		ctx.resourcesToUpdateLater() += _compute_tonemap;
	}

	void ColorCorrection::execute(ExecutionRecorder& exec)
	{
		if (_show_test_card)
		{
			exec(_render_test_card);
		}

		float window_exposure = 1.0;
		if (_target_window && _auto_fit_to_window)
		{
			window_exposure = _target_window->getColorCorrectionInfo().params.exposure;
		}

		float exposure = _exposure * window_exposure;

		bool enable = false;
		enable |= (_mode != ColorCorrectionMode::PassThrough);
		enable |= (exposure != 1.0f);
		enable |= (_src != _dst);
		if (enable)
		{
			struct PC
			{
				float exposure;
				float gamma;
			};
			
			PC pc{
				.exposure = exposure,
				.gamma = _gamma,
			};
			exec(_compute_tonemap->with(ComputeCommand::SingleDispatchInfo{
				.extent = _dst->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
		}
	}

	float ColorCorrection::computeTransferFunction(float linear) const
	{
		float res = linear;
		switch (_mode)
		{
		}
		return res;
	}

	void ColorCorrection::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			ImGui::PushID(name().c_str());
	
			ImGui::Checkbox("Auto Fit to window: ", &_auto_fit_to_window);

			static thread_local ImGuiListSelection gui_mode = ImGuiListSelection::CI{
				.name = "Mode",
				.mode = ImGuiListSelection::Mode::Combo,
				.same_line = true,
				.options = {
					ImGuiListSelection::Option{
						.name = "None",
					},
					ImGuiListSelection::Option{
						.name = "ITU",
					},
					ImGuiListSelection::Option{
						.name = "sRGB",
					},
					ImGuiListSelection::Option{
						.name = "scRGB",
					},
					ImGuiListSelection::Option{
						.name = "BT1886",
					},
					ImGuiListSelection::Option{
						.name = "HybridLogGamma",
					},
					ImGuiListSelection::Option{
						.name = "PerceptualQuantization",
					},
					ImGuiListSelection::Option{
						.name = "DisplayP3",
					},
					ImGuiListSelection::Option{
						.name = "DCI_P3",
					},
					ImGuiListSelection::Option{
						.name = "LegacyNTSC",
					},
					ImGuiListSelection::Option{
						.name = "LegacyPAL",
					},
					ImGuiListSelection::Option{
						.name = "ST240",
					},
					ImGuiListSelection::Option{
						.name = "AdobeRGB",
					},
					ImGuiListSelection::Option{
						.name = "SonySLog",
					},
					ImGuiListSelection::Option{
						.name = "SonySLog2",
					},
					ImGuiListSelection::Option{
						.name = "ACEScc",
					},
					ImGuiListSelection::Option{
						.name = "ACEScct",
					},
					ImGuiListSelection::Option{
						.name = "Gamma",
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

			{
				float log_exposure = std::log2(_exposure);
				bool exposure_changed = false;
				exposure_changed = ImGui::SliderFloat("log2(Exposure)", &log_exposure, -5, 5, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
				//ImGui::SameLine();
				if (ImGui::Button("0"))
				{
					exposure_changed |= true;
					log_exposure = 0;
				}
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
					_exposure = std::exp2f(log_exposure);
				}
				ImGui::Text("Exposure: %f", _exposure);
			}

			if (_mode == ColorCorrectionMode::Gamma)
			{
				ImGui::PushID('G');
				changed |= ImGui::SliderFloat("Gamma", &_gamma, 0.1, 4);
				ImGui::PopID();
				ImGui::Text("Snap Gamma: ");
				std::array<float, 9> snap_values = {1.0 / 2.4, 1.0 / 2.2, 0.5, 1.0 / 1.2, 1.0, 1.2, 2.0, 2.2, 2.4};
				char str_buffer[64];
				for (size_t i = 0; i < snap_values.size(); ++i)
				{
					ImGui::SameLine();
					*std::format_to(str_buffer, "{:.2f}", snap_values[i]) = char(0);
					bool snap = ImGui::Button(str_buffer);
					if (snap)
					{
						_gamma = snap_values[i];
						changed |= true;
					}
				}
				ImGui::Separator();

			}


			//if (_plot_raw_radiance.size() != _plot_samples)
			//{
			//	changed = true;
			//	_plot_raw_radiance.resize(_plot_samples);
			//	_plot_corrected_radiance.resize(_plot_samples);
			//}

			//if (changed)
			//{
			//	for (size_t i = 0; i < _plot_samples; ++i)
			//	{
			//		float t = (float(i) + (float(i) / float(_plot_samples - 1))) / float(_plot_samples);
			//		_plot_raw_radiance[i] = std::lerp(_plot_min_radiance, _plot_max_radiance, t);
			//		_plot_corrected_radiance[i] = computeTransferFunction(_plot_raw_radiance[i]);
			//	}

			//}

			//ImGui::PlotLines("Gamma Correction Preview", _plot_corrected_radiance.data(), _plot_samples, 0, nullptr, 0, _plot_corrected_radiance.back(), ImVec2(0, 200));

			ImGui::Checkbox("Show Test Card", &_show_test_card);
			
			ImGui::PopID();
		}
	}
}