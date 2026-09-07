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
		_sets_layouts(ci.sets_layouts)
		//_p_renderer_available_requirements(ci.p_renderer_available_requirements)
	{
		if (!_accumation_format.hasValue())
		{
			_accumation_format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		_inputs.resize(static_cast<u32>(Input::_Count));

		const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_STORAGE_BIT;

		_output = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".Output",
			.type = ci.image_type,
			.format = _accumation_format,
			.extent = ci.extent,
			.mips = 1,
			.layers = ci.layers ? ci.layers : Dyn<u32>(u32(1)),
			.usage = usage,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		setFormat();

		const std::filesystem::path shaders = "RenderLibShaders:/RenderLib";

		Callback reset_callback{
			.callback = [this]() {_reset = true; },
			.id = this,
		};

		_output->setInvalidationCallback(reset_callback);

		_taau_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".Command",
			.shader_path = shaders / "TAAU/TAAU.comp.slang",
			.extent = _output->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.image = _output,
					.binding = 2,
				},
			},
			.definitions = [this](DefinitionsList & res){ 
				res.clear(); 
				//res.pushBackFormatted("TAAU_MODE {:d}", static_cast<int>(_mode));
				res.pushBackFormatted("IMAGE_FORMAT {:s}", _format_glsl);
			},
		});
	}

	void TemporalAntiAliasingAndUpscaler::setInput(Input input_id, std::shared_ptr<ImageView> const& img)
	{
		u32 index = static_cast<u32>(input_id);
		assert(index < _inputs.size32());
		std::shared_ptr<ImageView> & slot = _inputs[index];
		if (slot)
		{
			slot->removeInvalidationCallback(this);
		}
		slot = img;
		if (slot)
		{
			Callback cb{
				.callback = [this, input_id](){},
				.id = this,
			};
			slot->setInvalidationCallback(std::move(cb));
		}
	}
	std::shared_ptr<ImageView> const& TemporalAntiAliasingAndUpscaler::getInput(Input input_id) const
	{
		u32 index = static_cast<u32>(input_id);
		assert(index < _inputs.size32());
		return _inputs[index];
	}

	TemporalAntiAliasingAndUpscaler::~TemporalAntiAliasingAndUpscaler()
	{
		for (std::shared_ptr<ImageView> const& input : _inputs)
		{
			if (input)
			{
				input->removeInvalidationCallback(this);
			}
		}
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
			_taau_command->descriptorSet()->setBinding(1, 0, 1, &_inputs[static_cast<u32>(Input::Color)]);
			ctx.resourcesToUpdateLater() += _taau_command;
		}
	}

	void TemporalAntiAliasingAndUpscaler::execute(ExecutionRecorder& exec, Camera const& camera)
	{
		bool blit = _enable;
		if (_mode == Mode::Default)
		{
			const Matrix4f new_matrix = camera.getWorldToProj();
			TAAU_PushConstant pc{
				.new_sample_weight = _renew_rate,
				.flags = 0,
			};
			_reset |= new_matrix != _matrix;
			if (_reset)
			{
				pc.flags |= 0x1;
				_accumulated_samples = 0;
				_matrix = new_matrix;
				pc.new_sample_weight = 1;
			}
			else
			{
				float samples_alpha = 1.0 / (_accumulated_samples + 1.0);
				float min_alpha = _renew_rate;
				pc.new_sample_weight = std::max(min_alpha, samples_alpha);
			}
			exec(_taau_command->with(ComputeCommand::SingleDispatchInfo{
				.extent = _output->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
			++_accumulated_samples;
			blit = false;
			_reset = false;
		}

		if(blit)
		{
			BlitImage & blitter = application()->getPrebuiltTransferCommands().blit_image;

			exec(blitter.with(BlitImage::BlitInfo{
				.src = _inputs[static_cast<u32>(Input::Color)],
				.dst = _output,
			}));
		}
	}

	TemporalAntiAliasingAndUpscaler::Requirements TemporalAntiAliasingAndUpscaler::calcFrameRequirements()
	{
		Requirements res{};
		VkExtent3D extent = _output->image()->extent().value();
		Vector2u out_res(extent.width, extent.height);
		res.input_resolution = out_res;
		res.downscale = Vector2f::Ones();
		res.image_memory = 0;
		return res;
	}

	namespace GUI
	{
		class TemporalAntiAliasingAndUpscalerInspector : public Panel
		{
		protected:
			using TAAU = TemporalAntiAliasingAndUpscaler;
			using Mode = TAAU::Mode;
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
					.mode = ImGuiListSelection::Mode::Dropdown,
					.same_line = true,
					.labels = {"Default"},
					.default_index = 0,
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
					_target->_mode = static_cast<Mode>(_mode.index());
					_target->_reset = true;
				}
				if (static_cast<Mode>(_mode.index()) == Mode::Default)
				{
					{
						float samples = rcp(_target->_renew_rate);
						if (ImGui::SliderFloat("Max samples: ", &samples, 1, 128 * 128, "%.0f samples", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
						{
							_target->_renew_rate = rcp(samples);
						}
						ImGui::BeginDisabled();
						ImGui::InputInt("Accumulated samples: ", (int*)&_target->_accumulated_samples);
						ImGui::EndDisabled();
					}
					{
						float renew_rate = _target->_renew_rate;
						char format[] = "%.3f";
						{
							float scale = std::abs(std::log10(renew_rate));
							int iscale = std::clamp(int(scale + 0.2f) + 3, 1, 9);
							format[2] = '0' + iscale;
						}
						if (ImGui::SliderFloat("Renewing Rate", &renew_rate, 0, 1, format, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
						{
							_target->_renew_rate = renew_rate;
						}
					}
					GUI::DeclareDynamic("Accumulation Format", _target->_accumation_format, [&](const char* label, VkFormat& f)
					{
						return InspectVkEnum<VkFormat>(ctx, label, &f, _available_formats);
					});
				}
				ImGui::PushStyleColor(ImGuiCol_Text, ctx.style().warning_yellow);
				_target->_reset |= ImGui::Button("Reset");
				ImGui::PopStyleColor();

			}
		};
	}

	std::shared_ptr<GUI::Panel> TemporalAntiAliasingAndUpscaler::makeInspector(GUI::InspectorMakeInfo const& imi)
	{
		assert(imi.target.get() == this);
		return std::make_shared<GUI::TemporalAntiAliasingAndUpscalerInspector>(std::static_pointer_cast<TemporalAntiAliasingAndUpscaler>(imi.target));
	}
}