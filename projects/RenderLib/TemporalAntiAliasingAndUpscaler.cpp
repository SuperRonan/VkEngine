#include "TemporalAntiAliasingAndUpscaler.hpp"

#include <vkl/VkObjects/DetailedVkFormat.hpp>

#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/ImGuiDynamic.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>
#include <vkl/GUI/InspectorMakeInfo.hpp>

namespace vkl
{
	namespace taau
	{
		static const std::array _formats = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R64G64B64A64_SFLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, };
	}

	TemporalAntiAliasingAndUpscaler::TemporalAntiAliasingAndUpscaler(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_input(ci.input),
		_sets_layouts(ci.sets_layouts)
	{
		if (!_accumation_format.hasValue())
		{
			_accumation_format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_STORAGE_BIT;

		_output = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".Output",
			.type = _input->image()->type(),
			.format = _accumation_format,
			.extent = _input->image()->extent(),
			.mips = 1,
			.layers = _input->image()->layers(),
			.usage = usage,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		setFormat();

		const std::filesystem::path shaders = "RenderLibShaders:/RenderLib";

		Callback reset_callback{
			.callback = [this]() {_reset = true; },
			.id = this,
		};

		_input->setInvalidationCallback(reset_callback);
		_output->setInvalidationCallback(reset_callback);

		_taau_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".Command",
			.shader_path = shaders / "TAAU/TAAU.comp",
			.extent = _output->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.image = _input,
					.binding = 1,
				},
				Binding{
					.image = _output,
					.binding = 2,
				},
			},
			.definitions = [this](DefinitionsList & res){ 
				res.clear(); 
				res.pushBackFormatted("TAAU_MODE {:d}", static_cast<int>(_mode));
				res.pushBackFormatted("IMAGE_FORMAT {:s}", _format_glsl);
			},
		});
	}

	TemporalAntiAliasingAndUpscaler::~TemporalAntiAliasingAndUpscaler()
	{
		_input->removeInvalidationCallback(this);
		_output->removeInvalidationCallback(this);
	}

	void TemporalAntiAliasingAndUpscaler::setFormat()
	{
		const VkFormat f = _accumation_format.value();
		DetailedVkFormat detailed_format = DetailedVkFormat::Find(f);
		_format_glsl = detailed_format.getGLSLName();
	}

	void TemporalAntiAliasingAndUpscaler::updateResources(UpdateContext& ctx)
	{
		setFormat();
		_output->updateResources(ctx);

		if (_enable || ctx.updateAnyway())
		{
			ctx.resourcesToUpdateLater() += _taau_command;
		}
	}

	void TemporalAntiAliasingAndUpscaler::execute(ExecutionRecorder& exec, Camera const& camera)
	{
		if (_enable)
		{
			const Matrix4f new_matrix = camera.getWorldToProj();
			TAAU_PushConstant pc{
				.alpha = _alpha,
				.flags = 0,
			};
			_reset |= new_matrix != _matrix;
			if (_reset)
			{
				pc.flags |= 0x1;
				_accumulated_samples = 0;
				_matrix = new_matrix;
			}
			if (_mode == Mode::Accumulate)
			{
				float alpha = 1.0 / (_accumulated_samples + 1.0);
				alpha = std::max<float>(alpha, 1.0 / double(_max_samples));
				pc.alpha = 1.0 - alpha;
				++_accumulated_samples;
			}
			exec(_taau_command->with(ComputeCommand::SingleDispatchInfo{
				.extent = _output->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
			_reset = false;
		}
		else
		{
			BlitImage & blitter = application()->getPrebuiltTransferCommands().blit_image;

			exec(blitter.with(BlitImage::BlitInfo{
				.src = _input,
				.dst = _output,
			}));
		}
	}

	namespace GUI
	{
		class TemporalAntiAliasingAndUpscalerInspector : public Panel
		{
		protected:
			std::shared_ptr<TemporalAntiAliasingAndUpscaler> _target;
			ImGuiListSelection _mode;
			MyVector<EnumOption<VkFormat>> _available_formats;
		public:

			TemporalAntiAliasingAndUpscalerInspector(std::shared_ptr<TemporalAntiAliasingAndUpscaler> const& target) :
				Panel(target->application(), std::format("{}", target->name())),
				_target(target)
			{
				_mode = ImGuiListSelection::CI{
					.name = "Mode",
					.mode = ImGuiListSelection::Mode::RadioButtons,
					.same_line = true,
					.labels = {"Accumulate", "Alpha"},
					.default_index = 1,
				};

				const VkImageUsageFlags usage = _target->output()->image()->usage();
				const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;

				_available_formats.resize(taau::_formats.size());
				for (size_t i = 0; i < taau::_formats.size(); ++i)
				{
					const VkFormat f = taau::_formats[i];
					DetailedVkFormat d = DetailedVkFormat::Find(f);

					VkFormatProperties2 format_props{
						.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
						.pNext = nullptr,
					};
					vkGetPhysicalDeviceFormatProperties2(application()->physicalDevice(), f, &format_props);

					const bool can_use_format = format_props.formatProperties.optimalTilingFeatures & features;

					if (can_use_format)
					{
						VkPhysicalDeviceImageFormatInfo2 format_info{
							.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
							.pNext = nullptr,
							.format = f,
							.type = VK_IMAGE_TYPE_2D,
							.tiling = VK_IMAGE_TILING_OPTIMAL,
							.usage = usage,
							.flags = 0,
						};
						VkImageFormatProperties2 image_props{
							.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
							.pNext = nullptr,
						};
						vkGetPhysicalDeviceImageFormatProperties2(application()->physicalDevice(), &format_info, &image_props);
					}

					_available_formats[i].value = f;
					_available_formats[i].disabled = !can_use_format;
				}
			}

			virtual void declareInline(Context& ctx) override
			{
				ImGui::Checkbox("Enable", &_target->_enable);
				_mode.setIndex(static_cast<size_t>(_target->_mode));
				if (_mode.declare())
				{
					_target->_mode = static_cast<TemporalAntiAliasingAndUpscaler::Mode>(_mode.index());
					_target->_reset = true;
				}
				if (_mode.index() == 0)
				{
					ImGui::InputInt("Max samples: ", (int*)&_target->_max_samples);
					ImGui::BeginDisabled();
					ImGui::InputInt("Accumulated samples: ", (int*)&_target->_accumulated_samples);
					ImGui::EndDisabled();
				}
				else if (_mode.index() == 1)
				{
					float one_minus_alpha = 1.0 - _target->_alpha;
					if (ImGui::SliderFloat("Renew Rate", &one_minus_alpha, 0, 1, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
					{
						_target->_alpha = 1.0 - one_minus_alpha;
					}
				}
				ImGui::PushStyleColor(ImGuiCol_Text, ctx.style().warning_yellow);
				_target->_reset |= ImGui::Button("Reset");
				ImGui::PopStyleColor();

				GUI::DeclareDynamic("Accumulation Format", _target->_accumation_format, [&](const char* label, VkFormat& f)
				{
					return InspectVkEnum<VkFormat>(ctx, label, &f, _available_formats);
				});
			}
		};
	}

	std::shared_ptr<GUI::Panel> TemporalAntiAliasingAndUpscaler::makeInspector(GUI::InspectorMakeInfo const& imi)
	{
		assert(imi.target.get() == this);
		return std::make_shared<GUI::TemporalAntiAliasingAndUpscalerInspector>(std::static_pointer_cast<TemporalAntiAliasingAndUpscaler>(imi.target));
	}
}