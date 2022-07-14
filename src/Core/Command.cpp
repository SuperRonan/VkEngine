
#include "Command.hpp"
#include <cassert>

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

		// TODO Resolve named bindings
		//{
		//	_pipeline->program();
		//	for (size_t i = 0; i < N; ++i)
		//	{
		//		ResourceBinding& binding = _bindings[i];
		//		if (!binding.resolved())
		//		{
		//			assert(!binding.name().empty());
		//		}
		//	}
		//}

		std::vector<VkDescriptorBufferInfo> buffers;
		buffers.reserve(N);
		std::vector<VkDescriptorImageInfo> images;
		images.reserve(N);

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(N);

		for (size_t i = 0; i < _bindings.size(); ++i)
		{
			ResourceBinding & b = _bindings[i];
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = *_desc_sets[b.set()],
				.dstBinding = b.binding(),
				.dstArrayElement = 0, // TODO
				.descriptorCount = 1,
				.descriptorType = b.vkType(),
			};
			if (b.isBuffer())
			{
				assert(b.buffers().size() == 1);
				buffers.emplace_back(VkDescriptorBufferInfo{
					.buffer = *b.buffers().front(),
					.offset = 0,
					.range = VK_WHOLE_SIZE,
				});
				VkDescriptorBufferInfo& info = buffers.back();
				write.pBufferInfo = &info;
			}
			else if (b.isImage())
			{
				VkDescriptorImageInfo info;
				if (!b.samplers().empty())
				{
					info.sampler = *b.samplers().front();
				}
				if (!b.images().empty())
				{
					info.imageView = *b.images().front();
					if (b.type() == ResourceBinding::Type::SAMPLED_IMAGE || b.type() == ResourceBinding::Type::TEXTURE)
					{
						info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}
					else if (b.type() == ResourceBinding::Type::IMAGE)
					{
						// TODO VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL if readonly
						info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					}
					images.push_back(info);
					write.pImageInfo = &images.back();
				}
			}
			else
			{
				assert(false);
			}

			writes.push_back(write);
		}
	}
}