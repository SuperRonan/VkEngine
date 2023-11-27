#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/Execution/TextureFromFile.hpp>
#include <Core/Execution/SamplerLibrary.hpp>
#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	class Material : public VkObject
	{
	public:
		enum class Type : uint32_t {
			None = 0,
			PhysicallyBased = 1,
		};
	protected:

		Type _type = Type::None;

		bool _synch = true;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Type type = Type::None;
			bool synch = true;
		};
		using CI = CreateInfo;

		Material(CreateInfo const& ci);

		constexpr Type type()const
		{
			return _type;
		}

		constexpr bool isSynch()const
		{
			return _synch;
		}

		virtual void updateResources(UpdateContext & ctx) = 0;

		virtual void declareGui(GuiContext & ctx) = 0;

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) = 0;

		static std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(Type type, uint32_t offset);

		virtual ShaderBindings getShaderBindings(uint32_t offset) = 0;

		virtual void installResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t offset) = 0;

		virtual void removeResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set) = 0;
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
		
		bool _should_update_props_buffer = false;
		std::shared_ptr<Buffer> _props_buffer = nullptr;

		std::shared_ptr<Sampler> _sampler = nullptr;

		std::filesystem::path _albedo_path = {};
		std::unique_ptr<TextureFromFile> _albedo_texture = nullptr;
		
		bool _force_albedo_prop = false;

		Properties getProperties() const;

		bool useAlbedoTexture()const
		{
			bool can_texture = _albedo_texture && _albedo_texture->isReady();
			return can_texture && !_force_albedo_prop;
		}

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			vec3 albedo = vec3(0.7);
			std::shared_ptr<Sampler> sampler = nullptr;
			std::filesystem::path albedo_path = {};
			bool synch = true;
		};
		using CI = CreateInfo;
		
		PhysicallyBasedMaterial(CreateInfo const& ci);

		virtual ~PhysicallyBasedMaterial() override;

		virtual void declareGui(GuiContext & ctx) override;

		virtual void updateResources(UpdateContext& ctx) override;

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) override
		{
			return getSetLayoutBindingsStatic(offset);
		}

		static std::vector<DescriptorSetLayout::Binding> getSetLayoutBindingsStatic(uint32_t offset);

		virtual ShaderBindings getShaderBindings(uint32_t offset) override;

		virtual void installResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t offset) override;

		virtual void removeResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set) override;
	};

	using PBMaterial = PhysicallyBasedMaterial;


}