#include "Program.hpp"
#include <map>
#include <cassert>

namespace vkl
{
	ProgramInstance::ProgramInstance(CreateInfo const &ci) : 
		AbstractInstance(ci.app, ci.name),
		_provided_sets_layouts(ci.sets_layouts)
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

		if (all_bindings.empty())
		{

		}
		else
		{	
			uint32_t max_set_index = [&all_bindings]()
			{
				uint32_t res = 0;
				for (const auto& [s, sb] : all_bindings)
				{
					res = std::max(res, s);
				}
				return res;
			}();

			_reflection_sets_layouts.resize(max_set_index + 1);

			for (size_t s = 0; s < (max_set_index + 1); ++s)
			{
				std::shared_ptr<DescriptorSetLayout> & set_layout = _reflection_sets_layouts.getRef(s);
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

				VkDescriptorSetLayoutCreateFlags flags = 0;

				const bool push_desc = false;
				if (push_desc)
				{
					flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
				}
				else
				{
					flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
				}
			

				set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
					.app = application(),
					.name = name() + ".DescSetLayout",
					.flags = flags,
					.vk_bindings = bindings,
					.metas = metas,
					.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
				});
			}
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

	bool ProgramInstance::checkSetsLayoutsMatch(std::ostream * stream) const
	{
		bool res = true;
		for (size_t s = 0; s < _reflection_sets_layouts.size(); ++s)
		{
			if (!!_reflection_sets_layouts[s] && !!_provided_sets_layouts[s])
			{
				const bool enough_bindings = _provided_sets_layouts[s]->bindings().size() >= _reflection_sets_layouts[s]->bindings().size();
				res &= enough_bindings;
				if (!enough_bindings && !!stream)
				{
					*stream << "checking Program " << name() << " sets layouts match error: Not enough bindings in provided set layout " << s 
						<< ", found " << _provided_sets_layouts[s]->bindings().size() << " but expected a least " << _reflection_sets_layouts[s]->bindings().size() << "!\n";
				}
				// Check if the reflection descriptors are present at the right index in the provided one, and for the right stages
				if (res)
				{

					size_t pi = 0;
					for (size_t i = 0; i < _reflection_sets_layouts[s]->bindings().size(); ++i)
					{
						const VkDescriptorSetLayoutBinding & rb = _reflection_sets_layouts[s]->bindings()[i];
						while (true)
						{
						// TODO _provided are now optional
							if (_provided_sets_layouts[s]->bindings()[pi].binding == rb.binding)
							{
								break;
							}
							++pi;
							if (pi == _provided_sets_layouts[s]->bindings().size())
							{
								pi = -1;
								break;
							}
						}
						if (pi == -1)
						{
							res = false;
							if (!!stream)
							{
								*stream << "checking Program " << name() << " sets layouts match error: Could not find provided binding at (set = " 
									<< s << ", binding = " << rb.binding << ")!\n";
							}
							break;
						}
						const VkDescriptorSetLayoutBinding & pb = _provided_sets_layouts[s]->bindings()[pi];

						const bool same_type = rb.descriptorType == pb.descriptorType;
						const bool same_count = rb.descriptorCount == pb.descriptorCount;
						const bool has_stages = (rb.stageFlags & pb.stageFlags) == rb.stageFlags;

						res &= (same_type & same_count & has_stages);

						if (!same_type && !!stream)
						{
							*stream << "checking Program " << name() << " sets layouts match error at (set = " << s << ", binding = " << rb.binding << 
								"): Descriptor type does not match, provided " << pb.descriptorType << " but expected " << rb.descriptorType << "!\n";
						}
						if (!same_count && !!stream)
						{
							*stream << "checking Program " << name() << " sets layouts match error at (set = " << s << ", binding = " << rb.binding << 
								"): Descriptor count does not match, provided " << pb.descriptorCount << " but expected " << rb.descriptorCount << "!\n";
						}
						if (!has_stages && !!stream)
						{
							*stream << "checking Program " << name() << " sets layouts match error at (set = " << s << ", binding = " << rb.binding << 
								"): Provided stages " << pb.stageFlags << " doest not contain expected stages " << rb.stageFlags << "!\n";
						}

						if (!res)
						{
							break;
						}
					}
				}
			}
			if (!res)
			{
				break;
			}
		}
		return res;
	}


	void ProgramInstance::createLayout()
	{
		const bool reflection_ok = reflect();
		assert(reflection_ok);

		if (!reflection_ok)
		{
			return;
		}
		assert(checkSetsLayoutsMatch(&std::cerr));

		_sets_layouts.resize(std::max(_reflection_sets_layouts.size(), _provided_sets_layouts.size()));
		for (size_t i = 0; i < _sets_layouts.size(); ++i)
		{
			_sets_layouts.getRef(i) = _provided_sets_layouts.getSafe(i) ? _provided_sets_layouts.getSafe(i) : _reflection_sets_layouts.getSafe(i);
		}

		_layout = std::make_shared<PipelineLayout>(PipelineLayout::CI{
			.app = application(),
			.name = name() + ".layout",
			.sets = _sets_layouts.asVector(),
			.push_constants = _push_constants,
		});
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
		waitForInstanceCreationIFN();
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

	void Program::waitForInstanceCreationIFN()
	{
		if (_create_instance_task)
		{
			_create_instance_task->waitIFN();
			assert(_create_instance_task->isSuccess());
			_create_instance_task = nullptr;
		}
	}

	std::vector<std::shared_ptr<AsynchTask>> Program::getShadersTasksDependencies()const
	{
		std::vector<std::shared_ptr<AsynchTask>> res;
		for (std::shared_ptr<Shader> const& shader : _shaders)
		{
			if (shader->compileTask())
			{
				res.push_back(shader->compileTask());
			}
		}
		return res;
	}

	GraphicsProgramInstance::GraphicsProgramInstance(CreateInfoVertex const& ci) :
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
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
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
		_task(ci.task),
		_mesh(ci.mesh),
		_fragment(ci.fragment)
	{
		if(_task)			_shaders.push_back(_task);
		if(_mesh)			_shaders.push_back(_mesh);
		if (_fragment)		_shaders.push_back(_fragment);

		createLayout();
		
		extractLocalSizeIFP();
	}

	void GraphicsProgramInstance::extractLocalSizeIFP()
	{
		std::shared_ptr<ShaderInstance> & shader = _task ? _task : _mesh;
		if (shader)
		{
			const auto& refl = shader->reflection();
			const auto& lcl = refl.entry_points[0].local_size;
			_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
		}
	}

	GraphicsProgram::GraphicsProgram(CreateInfoVertex const& civ) :
		Program(civ.app, civ.name, civ.sets_layouts),
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
		Program(cim.app, cim.name, cim.sets_layouts),
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
		// Maybe deduce the priority from the shaders
		waitForInstanceCreationIFN();
		assert(_fragment);
		if (_vertex || _geometry)
		{
			assert(_vertex);

			_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = "Creating Program " + name(),
				.priority = TaskPriority::ASAP(),
				.lambda = [this]() {		
					_inst = std::make_shared<GraphicsProgramInstance>(GraphicsProgramInstance::CIV{
						.app = application(),
						.name = name(),
						.sets_layouts = _provided_sets_layouts,
						.vertex = _vertex->instance(),
						.tess_control = _tess_control ? _tess_control->instance() : nullptr,
						.tess_eval = _tess_eval ? _tess_eval->instance() : nullptr,
						.geometry = _geometry ?  _geometry->instance() : nullptr,
						.fragment = _fragment->instance(),
					});
					return AsynchTask::ReturnType{
						.success = true,
					};
				},
				.dependencies = getShadersTasksDependencies(),
			});
			application()->threadPool().pushTask(_create_instance_task);
		}
		else
		{
			assert(!!_mesh);

			_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = "Creating Program " + name(),
				.priority = TaskPriority::ASAP(),
				.lambda = [this]() {
					_inst = std::make_shared<GraphicsProgramInstance>(GraphicsProgramInstance::CIM{
						.app = application(),
						.name = name(),
						.sets_layouts = _provided_sets_layouts,
						.task = _task ? _task->instance() : nullptr,
						.mesh = _mesh->instance(),
						.fragment = _fragment->instance(),
					});
					return AsynchTask::ReturnType{
						.success = true,
					};
				},
				.dependencies = getShadersTasksDependencies(),
			});
			application()->threadPool().pushTask(_create_instance_task);
		}
	}

	ComputeProgramInstance::ComputeProgramInstance(CreateInfo const& ci):
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
		}),
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
		assert(lcl.x != 0);
		assert(lcl.y != 0);
		assert(lcl.z != 0);
		_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
	}

	ComputeProgram::ComputeProgram(CreateInfo const& ci) :
		Program(ci.app, ci.name, ci.sets_layouts),
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
		waitForInstanceCreationIFN();
		assert(_shader);
		_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
			.name = "Creating Program " + name(),
			.priority = TaskPriority::ASAP(),
			.lambda = [this]() {
				_inst = std::make_shared<ComputeProgramInstance>(ComputeProgramInstance::CI{
					.app = application(),
					.name = name(),
					.sets_layouts = _provided_sets_layouts,
					.shader = _shader->instance(),
				});
				return AsynchTask::ReturnType{
					.success = true,
				};
			},
			.dependencies = getShadersTasksDependencies(),
		});
		application()->threadPool().pushTask(_create_instance_task);
	}

	ComputeProgram::~ComputeProgram()
	{
		_shader->removeInvalidationCallbacks(this);
	}

}