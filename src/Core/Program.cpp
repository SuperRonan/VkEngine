#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	bool Program::reflect()
	{
		// all_bindings[set][binding]
		struct BindingWithMeta
		{
			VkDescriptorSetLayoutBinding binding;
			DescriptorSetLayout::BindingMeta meta;
		};
		std::map<uint32_t, std::map<uint32_t, BindingWithMeta>> all_bindings;

		const bool keep_unused_bindings = false;

		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const Shader& shader = *_shaders[sh];
			const auto& refl = shader.reflection();
			for (size_t s = 0; s < refl.descriptor_set_count; ++s)
			{
				const auto& set = refl.descriptor_sets[s];
				std::map<uint32_t, BindingWithMeta>& set_bindings = all_bindings[set.set];
				for (size_t b = 0; b < set.binding_count; ++b)
				{
					const auto& binding = *set.bindings[b];
					if (keep_unused_bindings || binding.accessed)
					{
						VkDescriptorSetLayoutBinding vkb = {
							.binding = binding.binding,
							.descriptorType = (VkDescriptorType)binding.descriptor_type,
							.descriptorCount = binding.count,
							.stageFlags = (VkShaderStageFlags)shader.stage(),
						};
						DescriptorSetLayout::BindingMeta meta;
						meta.name = binding.name;
						meta.access = [&]()
						{
							const VkDescriptorType type = vkb.descriptorType;
							VkAccessFlags res = VK_ACCESS_NONE_KHR;
							if ( // Must be read only
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
								type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR ||
								type == VK_DESCRIPTOR_TYPE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
								type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT
								) {
								res |= VK_ACCESS_SHADER_READ_BIT;
							}
							else
							{
								const uint32_t decoration = binding.type_description->decoration_flags;
								const bool readonly = decoration & SPV_REFLECT_DECORATION_NON_WRITABLE;
								const bool writeonly = false; // decoration& SPV_REFLECT_DECORATION_NONE; // TODO add writeonly to spirv reflect
								if (readonly == writeonly) // Kind of an adge case
								{
									res |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
								}
								else if (readonly)
								{
									res |= VK_ACCESS_SHADER_READ_BIT;
								}
								else if (writeonly)
								{
									res |= VK_ACCESS_SHADER_WRITE_BIT;
								}
							}
							return res;
						}();
						meta.layout = [&]()
						{
							VkImageLayout res = VK_IMAGE_LAYOUT_MAX_ENUM;
							VkDescriptorType type = vkb.descriptorType;
							if ( // Is image
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
								type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
							) {
								if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
									res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
								{
									if (meta.access == VK_ACCESS_SHADER_READ_BIT)
										res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
									else
										res = VK_IMAGE_LAYOUT_GENERAL;
								}
							}
							return res;
						}();
						if (set_bindings.contains(binding.binding))
						{
							BindingWithMeta& already = set_bindings[binding.binding];
							already.binding.stageFlags |= shader.stage();
							const bool same_count = (already.binding.descriptorCount == vkb.descriptorCount);
							const bool same_type = (already.binding.descriptorType == vkb.descriptorType);
							assert(same_count && same_type);
							if (!(same_count && same_type))	return false;
						}
						else
						{
							set_bindings[binding.binding].binding = vkb;
							set_bindings[binding.binding].meta = meta;
						}
					}
					else
					{
						int _ = 0;
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
			std::vector<DescriptorSetLayout::BindingMeta> metas;
			bindings.reserve(sb.size());
			metas.reserve(sb.size());
			for (const auto& [bdi, bd] : sb)
			{
				bindings.push_back(bd.binding);
				metas.push_back(bd.meta);
			}
			set_layout = std::make_shared<DescriptorSetLayout>(_app, bindings, metas);
		}


		

		// Push constants
		{
			_push_constants.resize(0);
			std::map<uint32_t, VkPushConstantRange> push_constants;

			for (size_t sh = 0; sh < _shaders.size(); ++sh)
			{
				const auto& refl = _shaders[sh]->reflection();

				for (uint32_t pc = 0; pc < refl.push_constant_block_count; ++pc)
				{
					const auto& p = refl.push_constant_blocks[pc];
					const uint32_t o = p.absolute_offset;
					VkPushConstantRange pcr{
						.stageFlags = (VkShaderStageFlags)_shaders[sh]->stage(),
						.offset = o,
						.size = p.size,
					};
					if (push_constants.contains(o))
					{
						push_constants[o].stageFlags |= pcr.stageFlags;
						// TODO check that both match
					}
					else
					{
						push_constants[o] = pcr;
					}
				}
			}
			_push_constants.resize(push_constants.size());
			std::transform(push_constants.cbegin(), push_constants.cend(), _push_constants.begin(), [](auto const& pc) {return pc.second; });
			// TODO Check integrity of the push constants
		}

		return true;
	}


	void Program::createLayout()
	{
		const bool reflection_ok = reflect();
		assert(reflection_ok);

		std::vector<VkDescriptorSetLayout> set_layouts(_set_layouts.size());
		for (size_t i = 0; i < set_layouts.size(); ++i)	set_layouts[i] = *_set_layouts[i];

		const VkPipelineLayoutCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = (uint32_t)set_layouts.size(),
			.pSetLayouts = set_layouts.data(),
			.pushConstantRangeCount = (uint32_t)_push_constants.size(),
			.pPushConstantRanges = _push_constants.data(),
		};

		_layout = PipelineLayout(_app, ci);
	}

	VkApplication* GraphicsProgram::CreateInfo::getApplication()const
	{
		if (!!_vertex)	return _vertex->application();
		if (!!_geometry)	return _geometry->application();
		else //if (!!_fragment)	
			return _fragment->application();
	}

	GraphicsProgram::GraphicsProgram(CreateInfo&& ci) :
		Program(ci.getApplication()),
		_ci(std::move(ci))
	{
		if (_ci._vertex)	_shaders.push_back(_ci._vertex);
		if (_ci._geometry)	_shaders.push_back(_ci._geometry);
		if (_ci._fragment)	_shaders.push_back(_ci._fragment);
		createLayout();
	}


	ComputeProgram::ComputeProgram(Shader&& shader) :
		Program(shader.application())
	{
		_shaders = { std::make_shared<Shader>(std::move(shader)) };
		createLayout();
		extractLocalSize();
	}

	void ComputeProgram::extractLocalSize()
	{
		const auto& refl = _shaders.front()->reflection();
		const auto& lcl = refl.entry_points[0].local_size;
		_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
	}
}