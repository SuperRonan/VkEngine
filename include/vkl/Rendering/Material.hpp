#pragma once

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/Execution/ResourcesHolder.hpp>
#include <vkl/Execution/SamplerLibrary.hpp>
#include <vkl/Rendering/Texture.hpp>
#include <vkl/IO/GuiContext.hpp>

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
	
		static constexpr const uint32_t MAX_TEXTURE_COUNT = 8;

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

		std::array<std::shared_ptr<Texture>, MAX_TEXTURE_COUNT> _textures = {};

		// Use a shared sampler for all textures 
		std::shared_ptr<Sampler> _sampler = {};

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Type type = Type::None;
			bool synch = true;
			std::shared_ptr<Sampler> sampler = {};
		};
		using CI = CreateInfo;

		Material(CreateInfo const& ci);

		virtual ~Material() override;

		constexpr Type type()const
		{
			return _type;
		}

		constexpr bool isSynch()const
		{
			return _synch;
		}

		virtual bool isOpaque() const
		{
			return false;
		}

		virtual void updateResources(UpdateContext & ctx);

		virtual void declareGui(GuiContext & ctx) = 0;

		virtual MyVector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) = 0;

		static MyVector<DescriptorSetLayout::Binding> getSetLayoutBindings(Type type, uint32_t offset);

		//virtual ShaderBindings getShaderBindings(uint32_t offset) = 0;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0, bool include_textures = true) = 0;

		// stack_on_array:
		// false -> textures are registered to binding + 0, binding + 1, ..., binding + i @ array_index
		// true -> textures are register to binding @ array_index + 0, array_index + 1, ..., array_index + i
		void registerTexturesToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index, bool stack_on_array);
		void unRegisterTexturesFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index, bool stack_on_array);

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, bool include_textures = true) = 0;

		virtual void callResourceUpdateCallbacks() = 0;

		constexpr const auto& textures() const
		{
			return _textures;
		}

		constexpr const auto& sampler() const
		{
			return _sampler;
		}

		TextureAndSampler getTextureAndSampler(uint id) const
		{
			return TextureAndSampler{
				.texture = _textures[id],
				.sampler = _sampler,
			};
		}
	};

	class PhysicallyBasedMaterial : public Material
	{
	public:

		enum class TextureSlot : uint32_t
		{
			AlbedoAlpha = 0,
			Normal = 1,
			Metallic = 2,
			Roughness = 2,
		};
		
		using vec3 = glm::vec3;
		using vec4 = glm::vec4;

		struct Properties
		{
			ubo_vec3 albedo;
			uint32_t flags;
			float metallic;
			float roughness;
			float cavity;
		};

		struct Flags
		{
			uint32_t type : 2;
			uint32_t textures : 5;
			uint32_t bsdf_hemispheres : 2;
		};

		void callRegistrationCallback(DescriptorSetAndPool::Registration& reg, bool include_textures);

		std::shared_ptr<Texture> const& getTexture(TextureSlot slot) const
		{
			return _textures[static_cast<uint32_t>(slot)];
		}

		bool textureIsReady(TextureSlot slot) const
		{
			std::shared_ptr<Texture> const& tex = getTexture(slot);
			return tex && tex->isReady();
		}

		

	protected:


		Dyn<vec3> _albedo = {};
		Dyn<float> _metallic = {};
		Dyn<float> _roughness = {};
		Dyn<float> _cavity = {};
		
		Properties _cached_props;

		bool _should_update_props_buffer = false;
		std::shared_ptr<Buffer> _props_buffer = nullptr;

		bool _force_albedo_prop = false;

		Properties getProperties() const;

		bool useAlbedoTexture()const
		{
			return !_force_albedo_prop && textureIsReady(TextureSlot::AlbedoAlpha);
		}

		bool useAlphaTexture() const;

		bool useNormalTexture()const
		{
			return textureIsReady(TextureSlot::Normal);
		}

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Dyn<vec3> albedo = {};
			Dyn<float> metallic = {};
			Dyn<float> roughness = {};
			Dyn<float> cavity = {};
			std::shared_ptr<Sampler> sampler = nullptr;
			std::shared_ptr<Texture> albedo_texture = nullptr;
			std::shared_ptr<Texture> normal_texture = nullptr;
			bool synch = true;
		};
		using CI = CreateInfo;
		
		PhysicallyBasedMaterial(CreateInfo const& ci);

		virtual ~PhysicallyBasedMaterial() override;

		virtual bool isOpaque() const override
		{
			return !useAlphaTexture();
		}

		virtual void declareGui(GuiContext & ctx) override;

		virtual void updateResources(UpdateContext& ctx) override;

		virtual MyVector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) override
		{
			return getSetLayoutBindingsStatic(offset);
		}

		static MyVector<DescriptorSetLayout::Binding> getSetLayoutBindingsStatic(uint32_t offset);

		//virtual ShaderBindings getShaderBindings(uint32_t offset) override;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0, bool include_textures = true) override;

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, bool include_textures = true) override;

		virtual void callResourceUpdateCallbacks() override;
	};

	using PBMaterial = PhysicallyBasedMaterial;


}