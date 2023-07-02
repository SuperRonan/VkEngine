#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	ProgramInstance::ProgramInstance(VkApplication * app, std::string const& name) : 
		AbstractInstance(app, name)
	{}

	ProgramInstance::~ProgramInstance()
	{
		callDestructionCallbacks();
	}

	bool ProgramInstance::reflect()
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
			const ShaderInstance& shader = *_shaders[sh];
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
							VkAccessFlags res = VK_ACCESS_2_NONE_KHR;
							if (
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || 
								type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT
							) {
								res |= VK_ACCESS_2_UNIFORM_READ_BIT;
							}
							else if ( // Must be read only
								type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR ||
								type == VK_DESCRIPTOR_TYPE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
							) {
								res |= VK_ACCESS_2_SHADER_READ_BIT;
							}
							else
							{
								const uint32_t decoration = binding.type_description->decoration_flags;
								const bool readonly = decoration & SPV_REFLECT_DECORATION_NON_WRITABLE;
								const bool writeonly = false; // decoration& SPV_REFLECT_DECORATION_NONE; // TODO add writeonly to spirv reflect
								if (readonly == writeonly) // Kind of an adge case
								{
									res |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
								}
								else if (readonly)
								{
									res |= VK_ACCESS_2_SHADER_READ_BIT;
								}
								else if (writeonly)
								{
									res |= VK_ACCESS_2_SHADER_WRITE_BIT;
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
									if (meta.access == VK_ACCESS_2_SHADER_READ_BIT)
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

		uint32_t max_set_index = [&all_bindings]()
		{
			uint32_t res = 0;
			for (const auto& [s, sb] : all_bindings)
			{
				res = std::max(res, s);
			}
			return res;
		}();

		_set_layouts.resize(max_set_index + 1);
		for (size_t s = 0; s < _set_layouts.size(); ++s)
		{
			std::shared_ptr<DescriptorSetLayout> & set_layout = _set_layouts[s];
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			std::vector<DescriptorSetLayout::BindingMeta> metas;
			if (all_bindings.contains(s))
			{
				const auto& sb = all_bindings[s];
				bindings.reserve(sb.size());
				metas.reserve(sb.size());
				for (const auto& [bdi, bd] : sb)
				{
					bindings.push_back(bd.binding);
					metas.push_back(bd.meta);
				}
			}

			set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = application(),
				.name = name() + "DescSetLayout",
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT ,
				.bindings = bindings,
				.metas = metas,
				.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			});
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


	void ProgramInstance::createLayout()
	{
		const bool reflection_ok = reflect();
		assert(reflection_ok);

		std::vector<VkDescriptorSetLayout> set_layouts;
		for (size_t i = 0; i < _set_layouts.size(); ++i)
		{
			if (_set_layouts[i])
			{
				set_layouts.push_back(*_set_layouts[i]);
			}
			else
			{
				set_layouts.push_back(VK_NULL_HANDLE);
			}
		}

		const VkPipelineLayoutCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = (uint32_t)set_layouts.size(),
			.pSetLayouts = set_layouts.data(),
			.pushConstantRangeCount = (uint32_t)_push_constants.size(),
			.pPushConstantRanges = _push_constants.data(),
		};

		_layout = std::make_shared<PipelineLayout>(_app, ci);
	}

	Program::~Program()
	{
		destroyInstance();
		for (auto& shader : _shaders)
		{
			shader->removeInvalidationCallbacks(this);
		}
	}

	void Program::addShadersInvalidationCallbacks()
	{
		Callback ic{
			.callback = [this]() {this->destroyInstance();},
			.id = this,
		};
		for (auto& shader : _shaders)
		{
			shader->addInvalidationCallback(ic);
		}
	}

	void Program::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	bool Program::updateResources(UpdateContext & ctx)
	{
		bool res = false;
		
		for (auto& shader : _shaders)
		{
			res |= shader->updateResources(ctx);
		}
		
		if (!_inst)
		{
			createInstance();
			res = true;

		}

		return res;
	}

	GraphicsProgramInstance::GraphicsProgramInstance(CreateInfoVertex const& ci):
		ProgramInstance(ci.app, ci.name),
		_vertex(ci.vertex),
		_tess_control(ci.tess_control),
		_tess_eval(ci.tess_eval),
		_geometry(ci.geometry),
		_fragment(ci.fragment)
	{
		if (_vertex)		_shaders.push_back(_vertex);
		if (_tess_control)	_shaders.push_back(_tess_control);
		if (_tess_eval)		_shaders.push_back(_tess_eval);
		if (_geometry)		_shaders.push_back(_geometry);
		if (_fragment)		_shaders.push_back(_fragment);
		
		createLayout();
	}

	GraphicsProgramInstance::GraphicsProgramInstance(CreateInfoMesh const& ci) :
		ProgramInstance(ci.app, ci.name),
		_task(ci.task),
		_mesh(ci.mesh),
		_fragment(ci.fragment)
	{
		if(_task)			_shaders.push_back(_task);
		if(_mesh)			_shaders.push_back(_mesh);
		if (_fragment)		_shaders.push_back(_fragment);

		createLayout();
	}

	void GraphicsProgramInstance::extractLocalSize()
	{
		std::shared_ptr<ShaderInstance> & shader = _task ? _task : _mesh;
		const auto& refl = shader->reflection();
		const auto& lcl = refl.entry_points[0].local_size;
		_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
	}

	GraphicsProgram::GraphicsProgram(CreateInfoVertex const& civ) :
		Program(civ.app, civ.name),
		_vertex(civ.vertex),
		_tess_control(civ.tess_control),
		_tess_eval(civ.tess_eval),
		_geometry(civ.geometry),
		_fragment(civ.fragment)
	{
		if (_vertex)
		{
			_shaders.push_back(_vertex);
		}
		if (_tess_control)
		{
			_shaders.push_back(_tess_control);
		}
		if (_tess_eval)
		{
			_shaders.push_back(_tess_eval);
		}
		if (_geometry)
		{
			_shaders.push_back(_geometry);
		}
		if (_fragment)
		{
			_shaders.push_back(_fragment);
		}

		addShadersInvalidationCallbacks();
	}

	GraphicsProgram::GraphicsProgram(CreateInfoMesh const& cim) :
		Program(cim.app, cim.name),
		_task(cim.task),
		_mesh(cim.mesh),
		_fragment(cim.fragment)
	{
		if (_task)
		{
			_shaders.push_back(_task);
		}
		if (_mesh)
		{
			_shaders.push_back(_mesh);
		}
		if (_fragment)
		{
			_shaders.push_back(_fragment);
		}

		addShadersInvalidationCallbacks();
	}

	GraphicsProgram::~GraphicsProgram()
	{

	}

	void GraphicsProgram::createInstance()
	{
		if (_vertex || _geometry)
		{
			assert(_vertex);
			assert(_fragment);
			_inst = std::make_shared<GraphicsProgramInstance>(GraphicsProgramInstance::CIV{
				.app = application(),
				.name = name(),
				.vertex = _vertex->instance(),
				.tess_control = _tess_control ? _tess_control->instance() : nullptr,
				.tess_eval = _tess_eval ? _tess_eval->instance() : nullptr,
				.geometry = _geometry ?  _geometry->instance() : nullptr,
				.fragment = _fragment->instance(),
			});
		}
		else
		{
			assert(!!_mesh);
			_inst = std::make_shared<GraphicsProgramInstance>(GraphicsProgramInstance::CIM{
				.app = application(),
				.name = name(),
				.task = _task ? _task->instance() : nullptr,
				.mesh = _mesh->instance(),
				.fragment = _fragment->instance(),
			});
		}
	}

	ComputeProgramInstance::ComputeProgramInstance(CreateInfo const& ci):
		ProgramInstance(ci.app, ci.name),
		_shader(ci.shader)
	{
		_shaders = { _shader };
		createLayout();
		
		extractLocalSize();
	}
	
	void ComputeProgramInstance::extractLocalSize()
	{
		const auto& refl = _shader->reflection();
		const auto& lcl = refl.entry_points[0].local_size;
		_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
	}

	ComputeProgram::ComputeProgram(CreateInfo const& ci) :
		Program(ci.app, ci.name),
		_shader(ci.shader)
	{
		_shaders = { _shader };

		Callback ic{
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		};
		_shader->addInvalidationCallback(ic);
	}

	void ComputeProgram::createInstance()
	{
		_inst = std::make_shared<ComputeProgramInstance>(ComputeProgramInstance::CI{
			.app = application(),
			.name = name(),
			.shader = _shader->instance(),
		});
	}

	ComputeProgram::~ComputeProgram()
	{
		_shader->removeInvalidationCallbacks(this);
	}

}