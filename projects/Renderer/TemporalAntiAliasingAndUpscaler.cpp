#include "TemporalAntiAliasingAndUpscaler.hpp"

#include <Core/VkObjects/DetailedVkFormat.hpp>

#include <Core/Execution/Executor.hpp>

#include <Core/Commands/PrebuiltTransferCommands.hpp>

namespace vkl
{
	TemporalAntiAliasingAndUpscaler::TemporalAntiAliasingAndUpscaler(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_input(ci.input),
		_sets_layouts(ci.sets_layouts)
	{
		_output = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".Output",
			.type = _input->image()->type(),
			.format = _input->image()->format(),
			.extent = _input->image()->extent(),
			.mips = 1,
			.layers = _input->image()->layers(),
			.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_STORAGE_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		setFormat();

		const std::filesystem::path shaders = application()->mountingPoints()["ProjectShaders"];

		_input->setInvalidationCallback(Callback{
			.callback = [this](){_reset = true;},
			.id = this,
		});

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
			.definitions = [this](DefinitionsList & res){ res.clear(); res.pushBackFormatted("IMAGE_FORMAT {:s}", _format_glsl);},
		});
	}

	TemporalAntiAliasingAndUpscaler::~TemporalAntiAliasingAndUpscaler()
	{
		_input->removeInvalidationCallback(this);
	}

	void TemporalAntiAliasingAndUpscaler::setFormat()
	{
		DetailedVkFormat detailed_format = DetailedVkFormat::Find(_output->format().value());
		_format_glsl = detailed_format.getGLSLName();
	}

	void TemporalAntiAliasingAndUpscaler::updateResources(UpdateContext& ctx)
	{
		setFormat();
		_output->updateResource(ctx);

		if (_enable || ctx.updateAnyway())
		{
			ctx.resourcesToUpdateLater() += _taau_command;
		}
	}

	void TemporalAntiAliasingAndUpscaler::execute(ExecutionRecorder& exec, Camera const& camera)
	{
		if (_enable)
		{
			Matrix4f new_matrix = camera.getWorldToProj();
			TAAU_PushConstant pc{
				.alpha = _alpha,
				.flags = 0,
			};
			_reset |= new_matrix != _matrix;
			if (_reset)
			{
				pc.flags |= 0x1;
				_matrix = new_matrix;
			}
			_reset = false;
			exec(_taau_command->with(ComputeCommand::SingleDispatchInfo{
				.extent = _output->image()->instance()->createInfo().extent,
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
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

	void TemporalAntiAliasingAndUpscaler::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(this);
		{
			ImGui::Checkbox("Enable", &_enable);
			float one_minus_alpha = 1.0 - _alpha;
			if (ImGui::SliderFloat("Renew Rate", &one_minus_alpha, 0, 1, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat))
			{
				_alpha = 1.0 - one_minus_alpha;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ctx.style().warning_yellow);
			_reset |= ImGui::Button("Reset");
			ImGui::PopStyleColor();
			
		}
		ImGui::PopID();
	}
}