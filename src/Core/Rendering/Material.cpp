#include "Material.hpp"
#include <imgui/imgui.h>

namespace vkl
{
	Material::Material(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type)
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
		Material(Material::CI{.app = ci.app, .name = ci.name, .type = Type::PhysicallyBased}),
		_albedo(ci.albedo),
		_sampler(ci.sampler),
		_albedo_texture(ci.albedo_texture)
	{
		_should_update_props_buffer = true;

		_props_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".props_buffer",
			.size = sizeof(Properties),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
	}

	PhysicallyBasedMaterial::Properties PhysicallyBasedMaterial::getProperties() const
	{
		uint32_t flags = Flags::NONE;

		if (!!_albedo_texture)
		{
			flags |= Flags::USE_ALBEDO_TEXTURE;
		}
		
		return Properties{
			.albedo = _albedo,
			.flags = flags,
		};
	}

	void PhysicallyBasedMaterial::declareImGui()
	{
		ImGui::ColorPicker3("albedo", &_albedo.x);
	}

	ResourcesToDeclare PhysicallyBasedMaterial::getResourcesToDeclare()
	{
		ResourcesToDeclare res;
		res += _props_buffer;
		return res;
	}

	ResourcesToUpload PhysicallyBasedMaterial::getResourcesToUpload()
	{
		ResourcesToUpload res;
		if (_should_update_props_buffer)
		{
			const Properties props = getProperties();
			res += ResourcesToUpload::BufferUpload{
				.sources = {PositionedObjectView{.obj = props, .pos = 0}},
				.dst = _props_buffer,
			};
		}
		return res;
	}

	void PhysicallyBasedMaterial::notifyDataIsUploaded()
	{
		_should_update_props_buffer = false;
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

		if (_albedo_texture)
		{
			res += Binding{
				.view = _albedo_texture,
				.sampler = _sampler,
				.binding = offset + 1,
			};
		}

		return res;
	}
}