#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ResourcesHolder.hpp>

namespace vkl
{
	class Material : public VkObject, public ResourcesHolder
	{
	public:
		enum Type : uint32_t {
			None = 0,
			PhysicallyBased = 1,
		};
	protected:

		Type _type = Type::None;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Type type = Type::None;
		};
		using CI = CreateInfo;

		Material(CreateInfo const& ci);


		virtual ResourcesToDeclare getResourcesToDeclare() override = 0;

		virtual ResourcesToUpload getResourcesToUpload() override = 0;

		virtual void notifyDataIsUploaded() override = 0;

		virtual void declareImGui() = 0;

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) = 0;

		static std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(Type type, uint32_t offset);

		virtual ShaderBindings getShaderBindings(uint32_t offset) = 0;
	};

	class PhysicallyBasedMaterial : public Material
	{
	public:
		
		using vec3 = glm::vec3;
		using vec4 = glm::vec4;

		struct Properties
		{
			alignas(16) vec3 albedo;
			uint32_t flags;
		};

		enum Flags : uint32_t {
			NONE = 0,
			USE_ALBEDO_TEXTURE = (1 << 1),
		};
	protected:


		vec3 _albedo;
		
		bool _should_update_props_buffer;
		std::shared_ptr<Buffer> _props_buffer;

		Properties getProperties() const;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			vec3 albedo = vec3(0.7);
		};
		using CI = CreateInfo;
		
		PhysicallyBasedMaterial(CreateInfo const& ci);


		virtual void declareImGui() override;

		virtual ResourcesToDeclare getResourcesToDeclare() override;

		virtual ResourcesToUpload getResourcesToUpload() override;

		virtual void notifyDataIsUploaded() override;

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) override
		{
			return getSetLayoutBindingsStatic(offset);
		}

		static std::vector<DescriptorSetLayout::Binding> getSetLayoutBindingsStatic(uint32_t offset);

		virtual ShaderBindings getShaderBindings(uint32_t offset) override;
	};

	using PBMaterial = PhysicallyBasedMaterial;
}