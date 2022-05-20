#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	bool Program::buildSetLayouts()
	{
		// all_bindings[set][binding]
		std::map<uint32_t, std::map<uint32_t, VkDescriptorSetLayoutBinding>> all_bindings;
		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const Shader& shader = *_shaders[sh];
			assert(shader.reflection());
			const auto refl = shader.reflection();
			for (size_t s = 0; s < refl.descriptor_set_count; ++s)
			{
				const auto& set = refl.descriptor_sets[s];
				std::map<uint32_t, VkDescriptorSetLayoutBinding> & set_bindings = all_bindings[set.set];
				for (size_t b = 0; b < set.binding_count; ++b)
				{
					const auto& binding = *set.bindings[b];
					VkDescriptorSetLayoutBinding vkb = {
						.binding = binding.binding,
						.descriptorType = (VkDescriptorType) binding.descriptor_type,
						.descriptorCount = binding.count,
						.stageFlags = (VkShaderStageFlags) shader.stage(),
					};
					if (set_bindings.contains(binding.binding))
					{
						VkDescriptorSetLayoutBinding& already = set_bindings[binding.binding];
						already.stageFlags |= shader.stage();
						const bool same_count = (already.descriptorCount == vkb.descriptorCount);
						const bool same_type = (already.descriptorType == vkb.descriptorType);
						assert(same_count && same_type);
						if (!(same_count && same_type))	return false;
					}
					else
					{
						set_bindings[binding.binding] = vkb;
					}
				}
			}
		}

		_set_layouts.resize(all_bindings.size());
		for (auto& [s, sb] : all_bindings)
		{
			assert(s < _set_layouts.size());
			std::shared_ptr<DescriptorSetLayout> & set_layout = _set_layouts[s];
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(sb.size());
			for (const auto& [bdi, bd] : sb)
			{
				bindings.push_back(bd);
			}
			set_layout = std::make_shared<DescriptorSetLayout>(_app, bindings);
		}
		return true;
	}

	void Program::createLayout()
	{
		std::vector<std::vector<DescriptorSetLayout>> descriptors_per_shader(_shaders.size());
		for (size_t s = 0; s < _shaders.size(); ++s)
		{
			descriptors_per_shader[s] = _shaders[s]->descriptorLayouts();
		}

		VkPipelineLayoutCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			
		};
	}
}