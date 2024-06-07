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
		struct BindingInfo 
		{
			std::string name = {};
			uint32_t binding = 0;
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			uint32_t count = 0;
			VkShaderStageFlags stages = 0;
			//std::vector<std::shared_ptr<Sampler>> immutable_samplers = {}; // TODO
			VkAccessFlagBits2 access = VK_ACCESS_2_NONE;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkFlags usage = 0;

			DescriptorSetLayout::Binding asDSLBinding() const
			{
				return DescriptorSetLayout::Binding{
					.name = name,
					.binding = binding,
					.type = type,
					.count = count,
					.stages = stages,
					.access = access,
					.layout = layout,
					.usage = usage,
				};
			}

			VkDescriptorSetLayoutBinding getVkBinding()const
			{
				return VkDescriptorSetLayoutBinding{
					.binding = binding,
					.descriptorType = type,
					.descriptorCount = count,
					.stageFlags = stages,
					.pImmutableSamplers = nullptr,
				};
			}

			DescriptorSetLayoutInstance::BindingMeta getMeta()const
			{
				return DescriptorSetLayoutInstance::BindingMeta{
					.name = name,
					.access = access,
					.layout = layout,
					.usage = usage,
				};
			}
		};
		std::map<uint32_t, std::map<uint32_t, BindingInfo>> all_bindings;

		const bool keep_unused_bindings = false;

		//if (name() == "")
		//{
		//	VKL_BREAKPOINT_HANDLE;
		//}

		for (size_t sh = 0; sh < _shaders.size(); ++sh)
		{
			const ShaderInstance& shader = *_shaders[sh];
			const auto& refl = shader.reflection();
			for (size_t s = 0; s < refl.descriptor_set_count; ++s)
			{
				const auto& set = refl.descriptor_sets[s];
				std::map<uint32_t, BindingInfo>& set_bindings = all_bindings[set.set];
				for (size_t b = 0; b < set.binding_count; ++b)
				{
					const auto& binding = *set.bindings[b];
					if (keep_unused_bindings || binding.accessed)
					{
						BindingInfo binding_info{
							.name = binding.name,
							.binding = binding.binding,
							.type = (VkDescriptorType)binding.descriptor_type,
							.count = binding.count,
							.stages = (VkShaderStageFlags)shader.stage(),
						};
						binding_info.access = [&]()
						{
							const VkDescriptorType type = binding_info.type;
							VkAccessFlags res = VK_ACCESS_2_NONE_KHR;
							if (
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
								type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || 
								type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT
							) {
								res |= VK_ACCESS_2_UNIFORM_READ_BIT;
							}
							else if ( // Must be read only
								type == VK_DESCRIPTOR_TYPE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
							) {
								res |= VK_ACCESS_2_SHADER_READ_BIT;
							}
							else if (type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
							{
								res |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
							}
							else if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
							{
								res |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
							}
							else if(
								type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || 
								type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC || 
								type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || 
								type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
							) {
								uint32_t d1 = binding.decoration_flags;
								uint32_t d2 = binding.type_description ? binding.type_description->decoration_flags : 0;
								uint32_t d3 = binding.block.decoration_flags;
								uint32_t d4 = binding.block.type_description ? binding.block.type_description->decoration_flags : 0;
								uint32_t decoration = d1;
								if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
								{
									decoration = d3;
								}
								const bool readonly = decoration & SPV_REFLECT_DECORATION_NON_WRITABLE;
								const bool writeonly = decoration & SPV_REFLECT_DECORATION_NON_READABLE;
								if (readonly == writeonly) // Kind of an edge case
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
							else
							{
								NOT_YET_IMPLEMENTED;
							}
							return res;
						}();
						binding_info.layout = [&]()
						{
							// Note: with VK_EXT_attachment_feedback_loop_layout, the layout may be VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT
							// TODO handle this case (maybe manually do it, since this case would be very rare)
							VkImageLayout res = VK_IMAGE_LAYOUT_MAX_ENUM;
							VkDescriptorType type = binding_info.type;
							if ( // Is image
								type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
								type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
							) {
								res = application()->options().getLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);
							}
							else if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
							{
								// color or depth attachment does not seem to matter here
								// Plus this layout has to match the one in the RenderPass
								res = application()->options().getLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
							}
							else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
							{
								// It appears readonly storage image must be in general layout, shader read only optimal is not allowed
								//if (binding_info.access == VK_ACCESS_2_SHADER_READ_BIT)
								//	res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								//else
								//	res = VK_IMAGE_LAYOUT_GENERAL;
								res = VK_IMAGE_LAYOUT_GENERAL;
							}
							return res;
						}();
						if (set_bindings.contains(binding.binding))
						{
							BindingInfo& already = set_bindings[binding.binding];
							already.stages |= shader.stage();
							const bool same_count = (already.count == binding_info.count);
							const bool same_type = (already.type == binding_info.type);
							assert(same_count && same_type);
							if (!(same_count && same_type))	return false;
						}
						else
						{
							set_bindings[binding.binding] = binding_info;
						}
					}
					else
					{
						VKL_BREAKPOINT_HANDLE
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
				std::shared_ptr<DescriptorSetLayoutInstance> & set_layout = _reflection_sets_layouts.getRef(s);
				MyVector<VkDescriptorSetLayoutBinding> vk_bindings;
				MyVector<DescriptorSetLayoutInstance::BindingMeta> metas;
				if (all_bindings.contains(s))
				{
					const auto& sb = all_bindings[s];
					vk_bindings.reserve(sb.size());
					metas.reserve(sb.size());
					for (const auto& [bdi, bd] : sb)
					{
						vk_bindings.push_back(bd.getVkBinding());
						metas.push_back(bd.getMeta());
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
			

				set_layout = std::make_shared<DescriptorSetLayoutInstance>(DescriptorSetLayoutInstance::CI{
					.app = application(),
					.name = name() + ".DescSetLayout",
					.flags = flags,
					.vk_bindings = std::move(vk_bindings),
					.metas = std::move(metas),
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
				DescriptorSetLayoutInstance & refl_layout = *_reflection_sets_layouts[s];
				DescriptorSetLayoutInstance & provided_layout = *_provided_sets_layouts[s];
				const bool enough_bindings = provided_layout.bindings().size() >= refl_layout.bindings().size();
				res &= enough_bindings;
				if (!enough_bindings && !!stream)
				{
					*stream << "checking Program " << name() << " sets layouts match error: Not enough bindings in provided set layout " << s 
						<< ", found " << provided_layout.bindings().size() << " but expected a least " << refl_layout.bindings().size() << "!\n";
				}
				// Check if the reflection descriptors are present at the right index in the provided one, and for the right stages
				if (res)
				{

					size_t pi = 0; // provided i
					for (size_t i = 0; i < refl_layout.bindings().size(); ++i)
					{
						const VkDescriptorSetLayoutBinding & rb = refl_layout.bindings()[i];
						while (true)
						{
						// TODO _provided are now optional
							if (provided_layout.bindings()[pi].binding == rb.binding)
							{
								break;
							}
							++pi;
							if (pi == provided_layout.bindings().size())
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
						const VkDescriptorSetLayoutBinding & pb = provided_layout.bindings()[pi];

						const bool same_type = rb.descriptorType == pb.descriptorType;
						const bool same_count = (rb.descriptorCount == 0) || rb.descriptorCount == pb.descriptorCount;
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
		assert(checkSetsLayoutsMatch(&std::cout));

		_sets_layouts.resize(std::max(_reflection_sets_layouts.size(), _provided_sets_layouts.size()));
		for (size_t i = 0; i < _sets_layouts.size(); ++i)
		{
			std::shared_ptr<DescriptorSetLayoutInstance> & sli = _sets_layouts.getRef(i);
			sli = _provided_sets_layouts.getSafe(i);
			if(!sli)
				sli = _reflection_sets_layouts.getSafe(i);
			if(!sli)
				sli = application()->getEmptyDescSetLayout();
		}

		_layout = std::make_shared<PipelineLayoutInstance>(PipelineLayoutInstance::CI{
			.app = application(),
			.name = name() + ".layout",
			.sets = _sets_layouts.asVector(),
			.push_constants = _push_constants,
		});
	}

	Program::Program(CreateInfo const& ci):
		ParentType(ci.app, ci.name, ci.hold_instance),
		_provided_sets_layouts(ci.sets_layouts)
	{}

	Program::~Program()
	{
		// TODO can cancel task
		destroyInstanceIFN();
		for (auto& shader : _shaders)
		{
			shader->removeInvalidationCallback(this);
		}
		for (size_t i = 0; i < _provided_sets_layouts.size(); ++i)
		{
			if (_provided_sets_layouts[i])
			{
				_provided_sets_layouts[i]->removeInvalidationCallback(this);
			}
		}
	}

	void Program::setInvalidationCallbacks()
	{
		Callback ic{
			.callback = [this]() {
				this->destroyInstanceIFN();
			},
			.id = this,
		};
		for (auto& shader : _shaders)
		{
			shader->setInvalidationCallback(ic);
		}
		for (size_t i = 0; i < _provided_sets_layouts.size(); ++i)
		{
			if (_provided_sets_layouts[i])
			{
				_provided_sets_layouts[i]->setInvalidationCallback(ic);
			}
		}
	}

	void Program::destroyInstanceIFN()
	{
		waitForInstanceCreationIFN();
		ParentType::destroyInstanceIFN();
	}

	bool Program::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		bool can_create = true;

		for (auto& shader : _shaders)
		{
			res |= shader->updateResources(ctx);
			can_create &= shader->hasInstanceOrIsPending();
		}
		
		if (checkHoldInstance())
		{
			if (!_inst && can_create)
			{
				createInstanceIFP();
				res = true;
			}
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
			.sets_layouts = ci.sets_layouts.getInstance(),
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
			.sets_layouts = ci.sets_layouts.getInstance(),
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
		Program(Program::CI{
			.app = civ.app, 
			.name = civ.name, 
			.sets_layouts = civ.sets_layouts,
			.hold_instance = civ.hold_instance,
		}),
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

		setInvalidationCallbacks();
	}

	GraphicsProgram::GraphicsProgram(CreateInfoMesh const& cim) :
		Program(Program::CI{
			.app = cim.app,
			.name = cim.name,
			.sets_layouts = cim.sets_layouts,
			.hold_instance = cim.hold_instance,
		}),
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

		setInvalidationCallbacks();
	}

	GraphicsProgram::~GraphicsProgram()
	{

	}

	void GraphicsProgram::createInstanceIFP()
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
			.sets_layouts = ci.sets_layouts.getInstance(),
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
		Program(Program::CI{
			.app = ci.app, 
			.name = ci.name, 
			.sets_layouts = ci.sets_layouts,
			.hold_instance = ci.hold_instance,
		}),
		_shader(ci.shader)
	{
		_shaders = { _shader };

		setInvalidationCallbacks();
	}

	void ComputeProgram::createInstanceIFP()
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
	}

}