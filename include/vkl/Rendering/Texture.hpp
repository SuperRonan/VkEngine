#pragma once

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/Execution/ResourcesHolder.hpp>
#include <vkl/VkObjects/DetailedVkFormat.hpp>
#include <filesystem>

#include <vkl/Execution/DescriptorSetsManager.hpp>

namespace vkl
{
	class GuiContext;

	class Texture : public VkObject
	{
	public:
		enum class MipsOptions
		{
			None = 0,
			Auto = 1,
			Load = 2,
			MAX_ENUM,
		};
	protected:

		bool _is_ready = false;

		that::FormatInfo _original_format = {};

		std::shared_ptr<ImageView> _view = nullptr;

		//std::vector<Callback> _resource_update_callback = {};

		MyVector<DescriptorSetAndPool::Registration> _registered_sets = {};

		void callRegistrationCallback(DescriptorSetAndPool::Registration & reg);

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		Texture(CreateInfo const& ci) : 
			VkObject(ci.app, ci.name)
		{}

		virtual void updateResources(UpdateContext& ctx) = 0;

		const std::shared_ptr<ImageView>& getView()const
		{
			return _view;
		}

		bool isReady() const
		{
			return _is_ready;
		}

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0);

		// Remove all registrations of set
		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set);
		// Remove a single registration corresponding to the params
		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index);

		virtual void callResourceUpdateCallbacks();

		struct MakeInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkFormat desired_format = VK_FORMAT_UNDEFINED;
			std::filesystem::path path = {};
			bool synch = true;
		};

		that::FormatInfo getOriginalFormat() const
		{
			return _original_format;
		}

		static std::shared_ptr<Texture> MakeShared(MakeInfo const& mi);

		 virtual void declareGUI(GuiContext & ctx);
	};
}