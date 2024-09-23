#include <vkl/Rendering/Material.hpp>
#include <imgui/imgui.h>

namespace vkl
{
	Material::Material(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type),
		_synch(ci.synch),
		_sampler(ci.sampler)
	{}

	Material::~Material()
	{
		for (size_t i = 0; i < _registered_sets; ++i)
		{
			auto & reg = _registered_sets[i];
			if (reg.include_textures)
			{
				for (size_t t = 0; t < _textures.size(); ++t)
				{
					std::shared_ptr<Texture> & tex = _textures[t];
					if (tex)
					{
						tex->unRegistgerFromDescriptorSet(reg.registration.set, reg.registration.binding, reg.registration.array_index);
					}
				}
			}
		}
	}

	std::vector<DescriptorSetLayout::Binding> Material::getSetLayoutBindings(Type type, uint32_t offset)
	{
		std::vector<DescriptorSetLayout::Binding> res;
		if (type == Type::PhysicallyBased)
		{
			res = PhysicallyBasedMaterial::getSetLayoutBindingsStatic(offset);
		}
		return res;
	}


	void Material::updateResources(UpdateContext& ctx)
	{
		for (size_t t = 0; t < _textures.size(); ++t)
		{
			std::shared_ptr<Texture> & tex = _textures[t];
			if (tex)
			{
				tex->updateResources(ctx);
			}
		}
		if (_sampler)
		{
			_sampler->updateResources(ctx);
		}
	}

	void Material::registerTexturesToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index, bool stack_on_array)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(_textures.size()); ++i)
		{
			std::shared_ptr<Texture> & tex = _textures[i];
			if (tex)
			{
				uint32_t b = binding;
				uint32_t ai = array_index;
				if (stack_on_array)
				{
					ai += i;
				}
				else
				{
					b += i;
				}
				tex->registerToDescriptorSet(set, b, ai);
			}
		}
	}

	void Material::unRegisterTexturesFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index, bool stack_on_array)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(_textures.size()); ++i)
		{
			std::shared_ptr<Texture>& tex = _textures[i];
			if (tex)
			{
				uint32_t b = binding;
				uint32_t ai = array_index;
				if (stack_on_array)
				{
					ai += i;
				}
				else
				{
					b += i;
				}
				tex->unRegistgerFromDescriptorSet(set, b, ai);
			}
		}
	}


	PhysicallyBasedMaterial::PhysicallyBasedMaterial(CreateInfo const& ci) :
		Material(Material::CI{
			.app = ci.app, 
			.name = ci.name, 
			.type = Type::PhysicallyBased, 
			.synch = ci.synch,
			.sampler = ci.sampler,
		}),
		_albedo(ci.albedo),
		_metallic(ci.metallic),
		_roughness(ci.roughness),
		_cavity(ci.cavity),
		_cached_props({})
	{
		if (ci.albedo_texture)
		{
			_textures[static_cast<uint>(TextureSlot::AlbedoAlpha)] = ci.albedo_texture;
		}
		if (ci.normal_texture)
		{
			_textures[static_cast<uint>(TextureSlot::Normal)] = ci.normal_texture;
		}

		if (!_roughness.hasValue())
		{
			_roughness = 1.0f;
		}
		if (!_metallic.hasValue())
		{
			_metallic = 0.0f;
		}
		_should_update_props_buffer = true;

		_props_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".props_buffer",
			.size = sizeof(Properties),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		if (std::any_of(_textures.begin(), _textures.end(), [](std::shared_ptr<Texture> const& tex){return tex.operator bool();}))
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
		}
	}

	bool PhysicallyBasedMaterial::useAlphaTexture() const
	{
		bool res = textureIsReady(TextureSlot::AlbedoAlpha);
		if (res)
		{
			res = getTexture(TextureSlot::AlbedoAlpha)->getOriginalFormat().channels == 4;
			// A RGB texture will be stored with a RGBA format
			//const VkFormat vk_format = _albedo_texture->getView()->format().value();
			//const DetailedVkFormat fmt = DetailedVkFormat::Find(vk_format);
			//res = fmt.color.channels == 4;
		}
		return res;
	}

	PhysicallyBasedMaterial::Properties PhysicallyBasedMaterial::getProperties() const
	{
		uint32_t flags_u32 = 0;
		Flags & flags = reinterpret_cast<Flags&>(flags_u32);

		flags.type = 1;

		if (useAlbedoTexture())
		{
			flags.textures |= (1 << 0);
		}

		if (useAlphaTexture())
		{
			flags.textures |= (1 << 1);
		}

		if (useNormalTexture())
		{
			flags.textures |= (1 << 2);
		}

		flags.bsdf_hemispheres |= 1;
		
		return Properties{
			.albedo = _albedo.valueOr(vec3(0)),
			.flags = flags_u32,
			.metallic = _metallic.valueOr(0),
			.roughness = _roughness.valueOr(0),
			.cavity = _cavity.valueOr(0),
		};
	}

	void PhysicallyBasedMaterial::declareGui(GuiContext & ctx)
	{
		
		ImGui::PushID(name().c_str());
		ImGui::Text("Name: ");
		ImGui::SameLine();
		ImGui::Text(name().c_str());

		const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float;

		auto declare_dynamic = [&]<class T>(Dyn<T> &dv, std::string_view label, auto const& imgui_f)
		{
			if (dv.hasValue())
			{
				T value = dv.value();
				ImGui::BeginDisabled(!dv.canSetValue());

				bool changed = imgui_f(&value);
				_should_update_props_buffer |= changed;
				if (changed)
				{
					dv.setValue(value);
				}

				ImGui::EndDisabled();
			}
			else
			{
				ImGui::Text("%s: no value!", label.data());
			}
		};

		auto declare_color = [&](Dyn<vec3>& dv, std::string_view label)
		{
			declare_dynamic(dv, label, [&](vec3* ptr) {
				return ImGui::ColorEdit3(label.data(), reinterpret_cast<float*>(ptr), color_flags);
			});
		};

		auto declare_float = [&](Dyn<float>& dv, std::string_view label, ImGuiSliderFlags flags = 0)
		{
			declare_dynamic(dv, label, [&](float* ptr) {
				return ImGui::SliderFloat(label.data(), ptr, 0, 1, "%.3f", flags);
			});
		};

		_should_update_props_buffer |= ImGui::Checkbox("Force Albedo property", &_force_albedo_prop);
		
		declare_color(_albedo, "albedo");


		declare_float(_metallic, "metallic", ImGuiSliderFlags_NoRoundToFormat);
		declare_float(_roughness, "roughness", ImGuiSliderFlags_NoRoundToFormat);
		declare_float(_cavity, "cavity", ImGuiSliderFlags_NoRoundToFormat);


		ImGui::PopID();
	}

	void PhysicallyBasedMaterial::updateResources(UpdateContext& ctx)
	{
		Material::updateResources(ctx);
		_props_buffer->updateResource(ctx);

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
			Material::registerTexturesToDescriptorSet(set, binding + 1, array_index, false);
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
						unRegisterTexturesFromDescriptorSet(set, reg.binding + 1, reg.array_index, false);
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