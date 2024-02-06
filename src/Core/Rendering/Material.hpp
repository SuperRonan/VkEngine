#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/Execution/SamplerLibrary.hpp>
#include <Core/Rendering/Texture.hpp>
#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	struct TextureAndSampler
	{
		std::shared_ptr<Texture> texture;
		std::shared_ptr<Sampler> sampler;
	};

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

		struct SetRegistration
		{
			DescriptorSetAndPool::Registration registration;
			bool include_textures;
		};
		MyVector<SetRegistration> _registered_sets = {};

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

		//virtual ShaderBindings getShaderBindings(uint32_t offset) = 0;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0, bool include_textures = true) = 0;

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, bool include_textures = true) = 0;

		virtual void callResourceUpdateCallbacks() = 0;
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
			USE_NORMAL_TEXTURE = (1 << 2),
		};

		void callRegistrationCallback(DescriptorSetAndPool::Registration& reg, bool include_textures);

	protected:


		vec3 _albedo;
		
		Properties _cached_props;

		bool _should_update_props_buffer = false;
		std::shared_ptr<Buffer> _props_buffer = nullptr;

		std::shared_ptr<Sampler> _sampler = nullptr;

		std::shared_ptr<Texture> _albedo_texture = nullptr;

		std::shared_ptr<Texture> _normal_texture = nullptr;
		
		bool _force_albedo_prop = false;

		Properties getProperties() const;

		bool useAlbedoTexture()const
		{
			bool can_texture = _albedo_texture && _albedo_texture->isReady();
			return can_texture && !_force_albedo_prop;
		}

		bool useNormalTexture()const
		{
			return _normal_texture && _normal_texture->isReady();
		}

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			vec3 albedo = vec3(0.7);
			std::shared_ptr<Sampler> sampler = nullptr;
			std::shared_ptr<Texture> albedo_texture = nullptr;
			std::shared_ptr<Texture> normal_texture = nullptr;
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

		//virtual ShaderBindings getShaderBindings(uint32_t offset) override;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0, bool include_textures = true) override;

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, bool include_textures = true) override;

		virtual void callResourceUpdateCallbacks() override;

		const std::shared_ptr<Sampler>& sampler()const
		{
			return _sampler;
		}

		TextureAndSampler albedoTextureAndSampler()
		{
			return TextureAndSampler{
				.texture = _albedo_texture,
				.sampler = _sampler,
			};
		}

		TextureAndSampler normalTextureAndSampler()
		{
			return TextureAndSampler{
				.texture = _normal_texture,
				.sampler = _sampler,
			};
		}

		const std::shared_ptr<Texture>& albedoTexture()const
		{
			return _albedo_texture;
		}

		const std::shared_ptr<Texture>& normalTexture()const
		{
			return _normal_texture;
		}
	};

	using PBMaterial = PhysicallyBasedMaterial;


}