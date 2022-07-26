#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	bool Program::buildSetLayouts()
	{
		// all_bindings[set][binding]
		struct BindingWithMeta
		{
			VkDescriptorSetLayoutBinding binding;
			DescriptorSetLayout::BindingMeta meta;
		};
		std::map<uint32_t, std::map<uint32_t, BindingWithMeta>> all_bindings;
		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const Shader& shader = *_shaders[sh];
			//assert(shader.reflection());
			const auto& reflector = shader.reflector();
			const auto & all_resources  = reflector.get_shader_resources();

			const auto addBindings_impl = [&](const spirv_cross::Resource* begin, const spirv_cross::Resource* end, VkDescriptorType type)
			{
				for (const spirv_cross::Resource* b = begin; b != end; ++b)
				{
					const bool is_push_constant = reflector.get_storage_class(b->id) == spv::StorageClassPushConstant;
					const bool accessed = true; // TODO
					if (is_push_constant)
					{
						// TODO
					}
					else
					{
						const uint32_t set = reflector.get_decoration(b->id, spv::DecorationDescriptorSet);
						const uint32_t binding = reflector.get_decoration(b->id, spv::DecorationBinding);
						const uint32_t count = 1; // TODO
						const std::string name = reflector.get_name(b->id);

						std::map<uint32_t, BindingWithMeta>& set_bindings = all_bindings[set];

						const VkAccessFlags access = [&]()
						{
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
								const bool readonly = reflector.get_decoration(b->id, spv::DecorationNonWritable);
								const bool writeonly = reflector.get_decoration(b->id, spv::DecorationNonReadable);
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

						const VkImageLayout layout = [&]()
						{
							VkImageLayout res = VK_IMAGE_LAYOUT_MAX_ENUM;
							if ( // Is image
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || 
								type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
							) {
								if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
									res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
								{
									if (access == VK_ACCESS_SHADER_READ_BIT)
										res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
									else
										res = VK_IMAGE_LAYOUT_GENERAL;
								}
							}
							return res;
						}();

						if (set_bindings.contains(binding))
						{
							BindingWithMeta& bwm = set_bindings[binding];
							bwm.binding.stageFlags |= (VkShaderStageFlags)shader.stage();
							bwm.meta.access |= access;
							if (bwm.meta.layout != layout)
							{
								bwm.meta.layout = VK_IMAGE_LAYOUT_GENERAL;
							}
						}
						else
						{
							const VkDescriptorSetLayoutBinding dslb{
								.binding = binding,
								.descriptorType = type,
								.descriptorCount = count,
								.stageFlags = (VkShaderStageFlags)shader.stage(),
							};
							BindingWithMeta bwm;
							bwm.binding = dslb;
							bwm.meta.name = name;
							bwm.meta.access = access;

							bwm.meta.layout = layout;
							set_bindings[binding] = bwm;
						}
					}
				}
			};

			const auto addBindings = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, VkDescriptorType type)
			{
				addBindings_impl(resources.data(), resources.data() + resources.size(), type);
			};
			
			addBindings(all_resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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
		return true;
	}

	bool Program::buildPushConstantRanges()
	{
		_push_constants.resize(0);
		std::map<uint32_t, VkPushConstantRange> res;
		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const auto& reflector = _shaders[sh]->reflector();
			
			for (uint32_t pc = 0; pc < refl.push_constant_block_count; ++pc)
			{
				const auto& p = refl.push_constant_blocks[pc];
				const uint32_t o = p.absolute_offset;
				VkPushConstantRange pcr{
					.stageFlags = (VkShaderStageFlags)_shaders[sh]->stage(),
					.offset = o,
					.size = p.size,
				};
				if (res.contains(o))
				{
					res[o].stageFlags |= pcr.stageFlags;
					// TODO check that both match
				}
				else
				{
					res[o] = pcr;
				}
			}
		}

		// TODO Check integrity of the push constants

		_push_constants.resize(res.size());
		std::transform(res.cbegin(), res.cend(), _push_constants.begin(), [](auto const& pc) {return pc.second; });

		return true;
	}


	void Program::createLayout()
	{
		const bool set_layouts_ok = buildSetLayouts();
		assert(set_layouts_ok);
		const bool push_constants_ok = buildPushConstantRanges();
		assert(push_constants_ok);

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