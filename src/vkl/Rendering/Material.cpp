#include <vkl/Rendering/Material.hpp>
#include <imgui/imgui.h>

namespace vkl
{
	Material::Material(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type),
		_synch(ci.synch)
	{}

	std::vector<DescriptorSetLayout::Binding> Material::getSetLayoutBindings(Type type, uint32_t offset)
	{
		std::vector<DescriptorSetLayout::Binding> res;
		if (type == Type::PhysicallyBased)
		{
			res = PhysicallyBasedMaterial::getSetLayoutBindingsStatic(offset);
		}
		return res;
	}


	PhysicallyBasedMaterial::PhysicallyBasedMaterial(CreateInfo const& ci) :
		Material(Material::CI{.app = ci.app, .name = ci.name, .type = Type::PhysicallyBased, .synch = ci.synch}),
		_albedo(ci.albedo),
		_sampler(ci.sampler),
		_albedo_texture(ci.albedo_texture),
		_normal_texture(ci.normal_texture),
		_cached_props({})
	{
		_should_update_props_buffer = true;

		_props_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".props_buffer",
			.size = sizeof(Properties),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		if (_albedo_texture || _normal_texture)
		{
			if (!_sampler)
			{
				_sampler = std::make_shared<Sampler>(Sampler::CI{
					.app = application(),
					.name = name() + ".sampler",
				});
			}
		}
	}

	PhysicallyBasedMaterial::~PhysicallyBasedMaterial()
	{
		for (size_t i = 0; i < _registered_sets; ++i)
		{
			auto & reg = _registered_sets[i];
			reg.registration.set->setBinding(reg.registration.binding, reg.registration.array_index, 1, (const BufferAndRange*)nullptr);
			if (reg.include_textures)
			{
				if (_albedo_texture)
				{
					_albedo_texture->unRegistgerFromDescriptorSet(reg.registration.set, reg.registration.binding, reg.registration.array_index);
				}
			}
		}
		_registered_sets.clear();
	}

	PhysicallyBasedMaterial::Properties PhysicallyBasedMaterial::getProperties() const
	{
		uint32_t flags = Flags::NONE;

		if (useAlbedoTexture())
		{
			flags |= Flags::USE_ALBEDO_TEXTURE;
		}

		if (useNormalTexture())
		{
			flags |= Flags::USE_NORMAL_TEXTURE;
		}
		
		return Properties{
			.albedo = _albedo,
			.flags = flags,
		};
	}

	void PhysicallyBasedMaterial::declareGui(GuiContext & ctx)
	{
		
		ImGui::PushID(name().c_str());
		ImGui::Text("Name: ");
		ImGui::SameLine();
		ImGui::Text(name().c_str());
		_should_update_props_buffer |= ImGui::Checkbox(" Force Albedo property", &_force_albedo_prop);
		if (!useAlbedoTexture())
		{
			_should_update_props_buffer |= ImGui::ColorPicker3("albedo", &_albedo.x);
		}


		ImGui::PopID();
	}

	void PhysicallyBasedMaterial::updateResources(UpdateContext& ctx)
	{
		_props_buffer->updateResource(ctx);
		if (_albedo_texture)
		{
			_albedo_texture->updateResources(ctx);
		}
		if (_normal_texture)
		{
			_normal_texture->updateResources(ctx);
		}
		if (_sampler)
		{
			_sampler->updateResources(ctx);
		}

		const Properties new_props = getProperties();
		if (new_props.flags != _cached_props.flags)
		{
			_should_update_props_buffer = true;
		}
		_cached_props = new_props;

		if (_should_update_props_buffer)
		{
			ResourcesToUpload::BufferSource source{
				.data = &_cached_props,
				.size = sizeof(_cached_props),
				.offset = 0,
				.copy_data = false,
			};
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = &source,
				.sources_count = 1,
				.dst = _props_buffer->instance(),
			};
			_should_update_props_buffer = false;
		}
	}

	std::vector<DescriptorSetLayout::Binding> PhysicallyBasedMaterial::getSetLayoutBindingsStatic(uint32_t offset)
	{
		std::vector<DescriptorSetLayout::Binding> res;
		using namespace std::containers_append_operators;
		res += DescriptorSetLayout::Binding{
			.name = "MaterialPropertiesBuffer",
			.binding = offset + 0,
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_2_UNIFORM_READ_BIT,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		};

		res += DescriptorSetLayout::Binding{
			.name = "AlbedoTexture",
			.binding = offset + 1,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
		};

		res += DescriptorSetLayout::Binding{
			.name = "NormalTexture",
			.binding = offset + 2,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.count = 1,
			.stages = VK_SHADER_STAGE_ALL,
			.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
		};

		return res;
	}

	//ShaderBindings PhysicallyBasedMaterial::getShaderBindings(uint32_t offset)
	//{
	//	ShaderBindings res;
	//	using namespace std::containers_append_operators;
	//	
	//	res += Binding{
	//		.buffer = _props_buffer,
	//		.binding = offset + 0,
	//	};

	//	return res;
	//}

	void PhysicallyBasedMaterial::callRegistrationCallback(DescriptorSetAndPool::Registration& reg, bool include_textures)
	{
		BufferAndRange _props_buffer_bar{
			.buffer = _props_buffer,
		};
		reg.set->setBinding(reg.binding + 0, reg.array_index, 1, &_props_buffer_bar);
			
		if (include_textures)
		{
			if (_sampler)
			{
				reg.set->setBinding(reg.binding + 1, reg.array_index, 1, nullptr, &_sampler);
				reg.set->setBinding(reg.binding + 2, reg.array_index, 1, nullptr, &_sampler);
			}
			// Automatically called by the texture
		}
	}

	void PhysicallyBasedMaterial::registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index, bool include_textures)
	{
		DescriptorSetAndPool::Registration reg{
			.set = set,
			.binding = binding,
			.array_index = array_index,
		};

		if (include_textures)
		{
			if(_albedo_texture)
			{
				_albedo_texture->registerToDescriptorSet(set, binding + 1, array_index);
			}
			if (_normal_texture)
			{
				_normal_texture->registerToDescriptorSet(set, binding + 2, array_index);
			}
		}

		callRegistrationCallback(reg, include_textures);
		_registered_sets += SetRegistration{.registration = std::move(reg), .include_textures = include_textures};
	}

	void PhysicallyBasedMaterial::unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, bool include_textures)
	{
		auto it = _registered_sets.begin();
		while (it != _registered_sets.end())
		{
			auto & reg = it->registration;
			if (it->registration.set == set)
			{
				{
					reg.set->setBinding(reg.binding, reg.array_index, 1, (const BufferAndRange*)nullptr);
					if (include_textures)
					{
						std::shared_ptr<ImageView> null_view = nullptr;
						std::shared_ptr<Sampler> null_sampler = nullptr;
						if (_albedo_texture)
						{
							_albedo_texture->unRegistgerFromDescriptorSet(reg.set, reg.binding, reg.array_index);
						}
						if (_normal_texture)
						{
							_normal_texture->unRegistgerFromDescriptorSet(reg.set, reg.binding, reg.array_index);
						}
						reg.set->setBinding(reg.binding + 1, reg.array_index, 1, &null_view, &null_sampler);
					}
				}
				it = _registered_sets.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void PhysicallyBasedMaterial::callResourceUpdateCallbacks()
	{
		for (auto& reg : _registered_sets)
		{
			callRegistrationCallback(reg.registration, reg.include_textures);
		}
	}
}