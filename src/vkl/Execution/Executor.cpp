#include <vkl/Execution/Executor.hpp>

#include <vkl/Utils/stl_extension.hpp>
#include <vkl/Rendering/DebugRenderer.hpp>
#include <random>

namespace vkl
{

	Executor::Executor(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_common_definitions(ci.common_definitions),
		_use_debug_renderer(ci.use_debug_renderer),
		_use_rt_pipeline(ci.use_ray_tracing_pipeline)
	{
		if (ci.common_ubo_size)
		{
			_common_ubo = std::make_shared<Buffer>(Buffer::CI{
				.app = application(),
				.name = name() + ".CommonUBO",
				.size = ci.common_ubo_size,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
		}

		if (_use_debug_renderer && !_common_ubo)
		{
			application()->logger()("The Debug Renderer requires using the common UBO!", Logger::Options::VerbosityMostImportant | Logger::Options::TagHighWarning);
			_use_debug_renderer = false;
		}
	}
	
	void Executor::buildCommonSetLayout()
	{
		using namespace std::containers_append_operators;
		MyVector<DescriptorSetLayout::Binding> bindings;

		if (_common_ubo)
		{
			bindings += DescriptorSetLayout::Binding{
				.name = "CommonUBOBinding",
				.binding = 0,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_UNIFORM_READ_BIT,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			};
		}

		if (_use_debug_renderer)
		{
			bindings += DebugRenderer::getLayoutBindings(1);
		}

		_common_set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
			.app = application(),
			.name = name() + ".common_layout",
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindings = bindings,
			.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
		});
	}

	void Executor::createDebugRenderer()
	{
		MultiDescriptorSetsLayouts sets_layouts;
		sets_layouts += {0, _common_set_layout};
		_debug_renderer = std::make_shared<DebugRenderer>(DebugRenderer::CreateInfo{
			.app = application(),
			.common_definitions = _common_definitions,
			.sets_layout = sets_layouts,
		});
	}

	void Executor::createCommonSet()
	{
		ShaderBindings bindings;

		using namespace std::containers_append_operators;
		
		if (_common_ubo)
		{
			bindings += Binding{
				.buffer = _common_ubo,
				.binding = 0,
			};
		}

		if (_use_debug_renderer)
		{
			bindings += _debug_renderer->getBindings(1);
		}

		_common_descriptor_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".common_set",
			.layout = _common_set_layout,
			.bindings = bindings,
		});
	}





	ExecutionRecorder::ExecutionRecorder(VkApplication * app, std::string const& name) :
		VkObject(app, name)
	{}


	void ExecutionRecorder::pushDebugLabel(std::string_view const& label, bool timestamp)
	{
		std::hash<std::string_view> hs;
		size_t seed = hs(label);
		auto rng = std::mt19937_64(seed);
		std::uniform_real_distribution<float> distrib(0, 1);
		vec4 color;
		color.x() = distrib(rng);
		color.y() = distrib(rng);
		color.z() = distrib(rng);
		color.w() = 1;
		pushDebugLabel(label, color, timestamp);
	}
}