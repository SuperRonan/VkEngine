#pragma once

#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/VkObjects/DetailedVkFormat.hpp>
#include <filesystem>

namespace vkl
{
	class Texture : public VkObject
	{
	protected:

		bool _is_ready = false;

		std::shared_ptr<ImageView> _view = nullptr;

		std::vector<Callback> _resource_update_callback = {};

		virtual void callResourceUpdateCallbacks();

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

		virtual void addResourceUpdateCallback(Callback const& cb);

		virtual void removeResourceUpdateCallback(VkObject* id);


		struct MakeInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::optional<VkFormat> desired_format = {};
			std::filesystem::path path = {};
			bool synch = true;
		};

		static std::shared_ptr<Texture> MakeNew(MakeInfo const& mi);
	};
}