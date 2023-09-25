#include "PipelineLayout.hpp"

namespace vkl
{
	PipelineLayout::PipelineLayout(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_sets(ci.sets),
		_push_constants(ci.push_constants)
	{
		std::vector<VkDescriptorSetLayout> sets_layout(_sets.size());
		for(size_t i=0; i<_sets.size(); ++i)	sets_layout[i] = _sets[i] ? *_sets[i] : VK_NULL_HANDLE;
		VkPipelineLayoutCreateInfo vk_ci {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.setLayoutCount = static_cast<uint32_t>(_sets.size()),
			.pSetLayouts = sets_layout.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(_push_constants.size()),
			.pPushConstantRanges = _push_constants.data(),
		};
		create(vk_ci);
		setVkName();
	}

	PipelineLayout::~PipelineLayout()
	{
		if (_layout != VK_NULL_HANDLE)
		{
			destroy();
		}
	}

	void PipelineLayout::create(VkPipelineLayoutCreateInfo const& ci)
	{
		VK_CHECK(vkCreatePipelineLayout(_app->device(), &ci, nullptr, &_layout), "Failed to create a pipeline layout.");
	}

	void PipelineLayout::setVkName()
	{
		application()->nameObject(VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			.objectHandle = reinterpret_cast<uint64_t>(_layout),
			.pObjectName = name().c_str(),
		});
	}

	void PipelineLayout::destroy()
	{
		vkDestroyPipelineLayout(_app->device(), _layout, nullptr);
		_layout = VK_NULL_HANDLE;
	}
}