
#include "Command.hpp"

namespace vkl
{
	

	void ShaderCommand::createDescriptorSets()
	{
		std::vector<std::shared_ptr<DescriptorSetLayout>> set_layouts = _pipeline->program()->setLayouts();
		_desc_pools.resize(set_layouts.size());
		_desc_sets.resize(set_layouts.size());
		for (size_t i = 0; i < set_layouts.size(); ++i)
		{
			_desc_pools[i] = std::make_shared<DescriptorPool>(set_layouts[i]);
			_desc_sets[i] = std::make_shared<DescriptorSet>(set_layouts[i], _desc_pools[i]);
		}
		const size_t N = _bindings.size();

		// Resolve named bindings
		{
			_pipeline->program()
			for (size_t i = 0; i < N; ++i)
			{
				ResourceBinding& binding = _bindings[i];
				if (!binding.resolved())
				{
					assert(!binding.name().empty());

				}
			}
		}

		std::vector<VkDescriptorBufferInfo> buffers(N);
		std::vector<VkDescriptorImageInfo> images(N);

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(N);


	}
}