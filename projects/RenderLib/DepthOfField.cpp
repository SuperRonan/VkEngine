#include "DepthOfField.hpp"

#include <vkl/VkObjects/DetailedVkFormat.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

namespace vkl
{

	DepthOfField::DepthOfField(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_target(ci.target),
		_depth(ci.depth),
		_camera(ci.camera),
		_sets_layouts(ci.sets_layouts)
	{
		createInternals();
	}

	void DepthOfField::createInternals()
	{
		std::shared_ptr<Image> const& itarget = _target->image();
		Dyn<VkImageSubresourceRange> const& range = _target->range();
		_target_copy = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".TargetCopy",
			.type = VK_IMAGE_TYPE_2D,
			.format = _target->format(),
			.extent = itarget->extent(),
			.mips = 1,
			.layers = [this]() {
				if (_target->range())
				{
					return _target->range().value().levelCount;
				}
				else
				{
					return _target->image()->layers().value();
				}
			},
			.tiling = itarget->tiling(),
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".Sampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});

		_depth_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".DepthSampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});

		std::filesystem::path shaders = application()->mountingPoints()["RenderLibShaders"];

		_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".Command",
			.shader_path = shaders / "DOF/DepthOfField.comp",
			.extent = _target->image()->extent(),
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.image = _target,
					.binding = 0,
				},
				Binding{
					.image = _target_copy,
					.binding = 1
				},
				Binding{
					.sampler = _sampler,
					.binding = 2,
				},
				Binding{
					.image = _depth,
					.sampler = _depth_sampler,
					.binding = 3,
				},
			},
			.definitions = [this](DefinitionsList & res)	{
				res.clear();
				DetailedVkFormat df = DetailedVkFormat::Find(_target->format().value());
				res.pushBackFormatted("TARGET_FORMAT {}", df.getGLSLName());
			},
		});
	}
	
	void DepthOfField::updateResources(UpdateContext& ctx)
	{
		_target->updateResource(ctx);
		_target_copy->updateResource(ctx);
		_sampler->updateResources(ctx);
		_depth_sampler->updateResources(ctx);
		ctx.resourcesToUpdateLater() += _command;
	}

	void DepthOfField::record(ExecutionRecorder& exec)
	{
		bool execute = true;
		execute &= (_camera->type() == Camera::Type::Perspective && _camera->aperture() > 0.0f);
		if (execute)
		{
			exec(application()->getPrebuiltTransferCommands().copy_image(CopyImage::CopyInfo{
				.src = _target,
				.dst = _target_copy,
			}));

			struct PC {
				float z_near;
				float z_far;
				float aperture;
				float focal_length;
				float focal_distance;
			};
			PC pc{
				.z_near = _camera->zNear(),
				.z_far = _camera->zFar(),
				.aperture = _camera->aperatureRadiusUnit(),
				.focal_length = _camera->focalLength(),
				.focal_distance = _camera->focalDistance(),
			};
			exec(_command->with(ComputeCommand::SingleDispatchInfo{
				.extent = _target->image()->extent().value(),
				.dispatch_threads = true,
				.pc_data = &pc,
				.pc_size = sizeof(pc),
			}));
		}
	}

	void DepthOfField::declareGUI(GuiContext& ctx)
	{

	}
}


