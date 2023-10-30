#include "Executor.hpp"
#include <Core/Utils/stl_extension.hpp>
#include <Core/Rendering/DebugRenderer.hpp>

namespace vkl
{
	
	void Executor::buildCommonSetLayout()
	{
		using namespace std::containers_operators;
		std::vector<DescriptorSetLayout::Binding> bindings;

		if (_use_debug_renderer)
		{
			bindings += { 
				.vk_binding = VkDescriptorSetLayoutBinding{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr,
				},
				.meta = DescriptorSetLayout::BindingMeta{
					.name = "DebugBuffer",
					.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				},
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
			.common_definitions = &_common_definitions,
			.sets_layout = sets_layouts,
		});
	}

	void Executor::createCommonSet()
	{
		ShaderBindings bindings;

		using namespace std::containers_operators;
		
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





	ExecutionThread::ExecutionThread(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_context(ci.context)
	{}

	void ExecutionThread::execute(Command& cmd)
	{
		cmd.execute(*_context);
	}

	void ExecutionThread::execute(std::shared_ptr<Command> cmd)
	{
		execute(*cmd);
	}

	void ExecutionThread::execute(Executable const& executable)
	{
		executable(*_context);
	}

}