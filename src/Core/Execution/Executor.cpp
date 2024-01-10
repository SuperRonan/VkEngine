#include "Executor.hpp"
#include <Core/Utils/stl_extension.hpp>
#include <Core/Rendering/DebugRenderer.hpp>
#include <random>

namespace vkl
{
	
	void Executor::buildCommonSetLayout()
	{
		using namespace std::containers_append_operators;
		std::vector<DescriptorSetLayout::Binding> bindings;

		if (_use_debug_renderer)
		{
			bindings += DescriptorSetLayout::Binding{
				.name = "DebugBuffer",
				.binding = 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};
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
		
		if (_use_debug_renderer)
		{
			bindings += _debug_renderer->getBindings();
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


	void ExecutionRecorder::pushDebugLabel(std::string const& label)
	{
		std::hash<std::string> hs;
		size_t seed = hs(label);
		auto rng = std::mt19937_64(seed);
		std::uniform_real_distribution<float> distrib(0, 1);
		vec4 color;
		color.r = distrib(rng);
		color.g = distrib(rng);
		color.b = distrib(rng);
		color.a = 1;
		pushDebugLabel(label, color);
	}
}