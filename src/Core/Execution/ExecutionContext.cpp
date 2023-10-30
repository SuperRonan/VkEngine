#include "ExecutionContext.hpp"
#include <cassert>
#include <random>

namespace vkl
{
	ExecutionContext::ExecutionContext(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_command_buffer(ci.cmd),
		_resource_tid(ci.resource_tid),
		_staging_pool(ci.staging_pool),
		_mounting_points(ci.mounting_points),
		_graphics_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._gfx_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_GRAPHICS,
		}),
		_compute_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._cmp_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_COMPUTE,
		}),
		_ray_tracing_bound_sets(DescriptorSetsManager::CI{
			.app = application(),
			.name = name() + "._rt_bound_sets",
			.cmd = _command_buffer,
			.pipeline_binding = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		})
	{
		_can_push_vk_debug_label = application()->hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	void ExecutionContext::pushDebugLabel(std::string const& label, vec4 const& color)
	{
		_debug_labels.push(DebugLabel{
			.label = label,
			.color = color,
		});
		if (_can_push_vk_debug_label)
		{
			VkDebugUtilsLabelEXT vk_label{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
				.pNext = nullptr,
				.pLabelName = label.c_str(),
				.color = {color.r, color.g, color.b, color.a},
			};
			application()->extFunctions()._vkCmdBeginDebugUtilsLabelEXT(*_command_buffer, &vk_label);
		}
	}

	void ExecutionContext::pushDebugLabel(std::string const& label)
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

	void ExecutionContext::popDebugLabel()
	{
		_debug_labels.pop();
		if (_can_push_vk_debug_label)
		{
			application()->extFunctions()._vkCmdEndDebugUtilsLabelEXT(*_command_buffer);
		}
	}

	void ExecutionContext::insertDebugLabel(std::string const& label, vec4 const& color)
	{
		if (_can_push_vk_debug_label)
		{
			VkDebugUtilsLabelEXT vk_label{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
				.pNext = nullptr,
				.pLabelName = label.c_str(),
				.color = {color.r, color.g, color.b, color.a},
			};
			application()->extFunctions()._vkCmdInsertDebugUtilsLabelEXT(*_command_buffer, &vk_label);
		}
	}
}
