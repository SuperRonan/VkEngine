#include "Material.hpp"
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
		_albedo_path(ci.albedo_path)
	{
		_should_update_props_buffer = true;

		_props_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".props_buffer",
			.size = sizeof(Properties),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		if (!_albedo_path.empty())
		{
			_albedo_texture = Texture::MakeNew(Texture::MakeInfo{
				.app = application(),
				.name = name() + ".albedo_texture",
				.path = _albedo_path,
				.synch = _synch,
			});

			assert(!!_sampler);
		}
	}

	PhysicallyBasedMaterial::~PhysicallyBasedMaterial()
	{
		if (_albedo_texture)
		{
			_albedo_texture->removeResourceUpdateCallback(this);
		}
	}

	PhysicallyBasedMaterial::Properties PhysicallyBasedMaterial::getProperties() const
	{
		uint32_t flags = Flags::NONE;

		if (useAlbedoTexture())
		{
			flags |= Flags::USE_ALBEDO_TEXTURE;
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
		if (_sampler)
		{
			_sampler->updateResources(ctx);
		}

		if (_should_update_props_buffer)
		{
			const Properties props = getProperties();
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = {PositionedObjectView{.obj = props, .pos = 0}},
				.dst = _props_buffer->instance(),
			};
			_should_update_props_buffer = false;
		}
	}

	std::vector<DescriptorSetLayout::Binding> PhysicallyBasedMaterial::getSetLayoutBindingsStatic(uint32_t offset)
	{
		std::vector<DescriptorSetLayout::Binding> res;
		using namespace std::containers_operators;
		res += DescriptorSetLayout::Binding{
			.vk_binding = {
				.binding = offset + 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			},
			.meta = {
				.name = "MaterialPropertiesBuffer",
				.access = VK_ACCESS_2_UNIFORM_READ_BIT,
				.buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			},
		};

		res += DescriptorSetLayout::Binding{
			.vk_binding = {
				.binding = offset + 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			},
			.meta = {
				.name = "AlbedoTexture",
				.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image_usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			},
		};

		return res;
	}

	ShaderBindings PhysicallyBasedMaterial::getShaderBindings(uint32_t offset)
	{
		ShaderBindings res;
		using namespace std::containers_operators;
		
		res += Binding{
			.buffer = _props_buffer,
			.binding = offset + 0,
		};

		return res;
	}

	void PhysicallyBasedMaterial::installResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t offset)
	{
		if (_albedo_texture)
		{
			Callback cb{
				.callback = [set, offset, this]() {
					set->setBinding(ShaderBindingDescription{
						.view = _albedo_texture->getView(),
						.sampler = _sampler,
						.binding = offset + 1,
					});
					_should_update_props_buffer = true;
				},
				.id = set.get(),
			};
			_albedo_texture->addResourceUpdateCallback(cb);
		}
	}

	void PhysicallyBasedMaterial::removeResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set)
	{
		if (_albedo_texture)
		{
			_albedo_texture->removeResourceUpdateCallback(set.get());
		}
	}
}