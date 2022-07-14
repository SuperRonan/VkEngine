#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	bool Program::buildSetLayouts()
	{
		// all_bindings[set][binding]
		struct NamedBinding
		{
			std::string name;
			VkDescriptorSetLayoutBinding binding;
		};
		std::map<uint32_t, std::map<uint32_t, NamedBinding>> all_bindings;
		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const Shader& shader = *_shaders[sh];
			//assert(shader.reflection());
			const auto & refl = shader.reflection();
			for (size_t s = 0; s < refl.descriptor_set_count; ++s)
			{
				const auto& set = refl.descriptor_sets[s];
				std::map<uint32_t, NamedBinding> & set_bindings = all_bindings[set.set];
				for (size_t b = 0; b < set.binding_count; ++b)
				{
					const auto& binding = *set.bindings[b];
					binding.type_description->decoration_flags
					VkDescriptorSetLayoutBinding vkb = {
						.binding = binding.binding,
						.descriptorType = (VkDescriptorType) binding.descriptor_type,
						.descriptorCount = binding.count,
						.stageFlags = (VkShaderStageFlags) shader.stage(),
					};
					if (set_bindings.contains(binding.binding))
					{
						NamedBinding& already = set_bindings[binding.binding];
						already.binding.stageFlags |= shader.stage();
						const bool same_count = (already.binding.descriptorCount == vkb.descriptorCount);
						const bool same_type = (already.binding.descriptorType == vkb.descriptorType);
						assert(same_count && same_type);
						if (!(same_count && same_type))	return false;
					}
					else
					{
						set_bindings[binding.binding].binding = vkb;
						set_bindings[binding.binding].name = binding.name;
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
			std::vector<std::string> names;
			bindings.reserve(sb.size());
			names.reserve(sb.size());
			for (const auto& [bdi, bd] : sb)
			{
				bindings.push_back(bd.binding);
				names.push_back(bd.name);
			}
			set_layout = std::make_shared<DescriptorSetLayout>(_app, bindings, names);
		}
		return true;
	}

	bool Program::buildPushConstantRanges()
	{
		_push_constants.resize(0);
		std::map<uint32_t, VkPushConstantRange> res;
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