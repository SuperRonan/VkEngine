#include "AmbientOcclusion.hpp"
#include <Core/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	AmbientOcclusion::AmbientOcclusion(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_positions(ci.positions),
		_normals(ci.normals)
	{
		
		assert(!!_positions);

		createInternalResources();
	}

	void AmbientOcclusion::createInternalResources()
	{
		_format = VK_FORMAT_R16_SNORM;
		_format_glsl = DetailedVkFormat::Find(_format).getGLSLName();

		Dyn<VkExtent3D> target_extent = [this]()
		{
			VkExtent3D ref = _positions->image()->extent().value();
			return VkExtent3D{
				.width = ref.width / _downscale,
				.height = ref.height / _downscale,
				.depth = ref.depth,
			};
		};

		_target = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".target",
			.type = VK_IMAGE_TYPE_2D,
			.format = &_format,
			.extent = target_extent,
			.layers = _positions->image()->layers(),
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_BITS,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".sampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		});

		const std::filesystem::path folder = ENGINE_SRC_PATH "/Shaders/Rendering/AmbientOcclusion/";

		Dyn<std::vector<std::string>> defs = [this]()
		{
			std::vector<std::string> res;
			res.push_back("OUT_FORMAT "s + _format_glsl);
			res.push_back("AO_SAMPLES "s + std::to_string(_ao_samples));
			return res;
		};

		ShaderBindings bindings = {
			Binding{
				.view = _target,
				.binding = 0,
			},
			Binding{
				.view = _positions,
				.sampler = _sampler,
				.binding = 1,
			},
		};

		if (_normals)
		{
			bindings.push_back(Binding{
				.view = _normals,
				.sampler = _sampler,
				.binding = 2,
			});
		}

		_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".command",
			.shader_path = folder / "AmbientOcclusion.comp",
			.extent = _target->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = std::move(bindings),
			.definitions = std::move(defs),
		});
	}


	void AmbientOcclusion::updateResources(UpdateContext& ctx)
	{
		_target->updateResource(ctx);
		if (_enable)
		{
			_sampler->updateResources(ctx);
			ctx.resourcesToUpdateLater() += _command;
		}
	}

	void AmbientOcclusion::execute(ExecutionRecorder& recorder, const Camera& camera)
	{
		if (_enable)
		{
			//recorder.pushDebugLabel(name());
				
			uint32_t flags = 0;

			if (_normals)
			{
				flags |= 1;
			}

			CommandPC pc{
				.camera_position = camera.position(),
				.flags = flags, 
				.radius = _radius,
			};

			recorder(_command->with(ComputeCommand::SingleDispatchInfo{
				.pc = pc,
			}));

			//recorder.popDebugLabel();
		}
	}

	void AmbientOcclusion::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());

		ImGui::Checkbox("Enable", &_enable);
		ImGui::InputInt("Samples", &_ao_samples);
		ImGui::SliderFloat("Radius", &_radius, 0, 0.2);

		ImGui::PopID();
	}
}