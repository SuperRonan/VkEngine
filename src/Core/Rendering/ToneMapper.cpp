#include "ToneMapper.hpp"
#include <imgui/imgui.h>
#include <Core/Execution/SamplerLibrary.hpp>
#include <Core/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	ToneMapper::ToneMapper(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_src(ci.src),
		_dst(ci.dst),
		_sampler(ci.sampler),
		_sets_layouts(ci.sets_layouts)
	{
		if (!_src)
		{
			_src = _dst;
		}
		createInternalResources();
	}

	void ToneMapper::createInternalResources()
	{
		const bool use_separate_src = _src != _dst;
		const std::filesystem::path folder = ENGINE_SRC_PATH "/Shaders/ToneMap/";

		ShaderBindings bindings;
		bindings.push_back(Binding{
			.view = _dst,
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
				.view = _src,
				.sampler = _sampler,
				.binding = 1,
			});
		}

		Dyn<std::vector<std::string>> definitions = [this]() -> std::vector<std::string>
		{
			std::vector<std::string> res(2);
			res[0] = "USE_SEPARATE_SOURCE "s + ((_src == _dst) ? "0"s : "1"s);
			res[1] = "DST_FORMAT "s + _dst_glsl_format;
			return res;
		};

		_compute_tonemap = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".DirectToneMap",
			.shader_path = folder / "ToneMap.comp",
			.sets_layouts = _sets_layouts,
			.bindings = bindings,
			.definitions = definitions,
		});
	}

	void ToneMapper::updateResources(UpdateContext& ctx)
	{
		const DetailedVkFormat dst_format = DetailedVkFormat::Find(_dst->format().value());
		_dst_glsl_format = dst_format.getGLSLName();

		ctx.resourcesToUpdateLater() += _compute_tonemap;
	}

	void ToneMapper::execute(ExecutionRecorder& exec)
	{
		if (_enable)
		{
			exec(_compute_tonemap->with(ComputeCommand::SingleDispatchInfo{
				.extent = _dst->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc = ComputePC{
					.exposure = _exposure,
					.gamma = _gamma,
				},
			}));
		}
	}

	float ToneMapper::computeGammaCorrection(float x) const
	{
		return std::pow(x * _exposure, _gamma);
	}

	void ToneMapper::declareGui(GuiContext & ctx)
	{
		if (ImGui::CollapsingHeader(name().c_str()))
		{
			ImGui::PushID(name().c_str());
			
			ImGui::Checkbox("Enable", &_enable);

			bool changed = false;

			changed |= ImGui::SliderFloat("log2(Exposure)", &_log_exposure, -10, 10);
			ImGui::Text("Exposure: %f", _exposure);
			
			changed |= ImGui::SliderFloat("Gamma", &_gamma, 0.1, 4);

			ImGui::Text("Snap Gamma: ");
			ImGui::SameLine();
			bool snap_1 = ImGui::Button("1.0");
			ImGui::SameLine();
			bool snap_2_2 = ImGui::Button("2.2");
			if(snap_1)	_gamma = 1.0f, changed = true;
			if(snap_2_2)	_gamma = 2.2f, changed = true;

			_exposure = std::exp2f(_log_exposure);

			ImGui::Separator();

			if (_plot_raw_radiance.size() != _plot_samples)
			{
				changed = true;
				_plot_raw_radiance.resize(_plot_samples);
				_plot_gamma_radiance.resize(_plot_samples);
			}

			if (changed)
			{
				for (size_t i = 0; i < _plot_samples; ++i)
				{
					float t = (float(i) + (float(i) / float(_plot_samples - 1))) / float(_plot_samples);
					_plot_raw_radiance[i] = std::lerp(_plot_min_radiance, _plot_max_radiance, t);
					_plot_gamma_radiance[i] = computeGammaCorrection(_plot_raw_radiance[i]);
				}

			}

			ImGui::PlotLines("Gamma Correction Preview", _plot_gamma_radiance.data(), _plot_samples, 0, nullptr, 0, _plot_gamma_radiance.back(), ImVec2(0, 200));
			
			ImGui::PopID();
		}
	}
}