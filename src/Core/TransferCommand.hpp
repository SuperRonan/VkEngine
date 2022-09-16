#pragma once

#include "DeviceCommand.hpp"

namespace vkl
{
	class BlitImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageBlit> _regions;
		VkFilter _filter = VK_FILTER_NEAREST;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageBlit> regions = {};
			VkFilter filter = VK_FILTER_NEAREST;
		};
		using CI = CreateInfo;

		BlitImage(CreateInfo const& ci);

		void setImages(std::shared_ptr<ImageView> src, std::shared_ptr<ImageView> dst);

		void setRegions(std::vector<VkImageBlit> const& regions = {});

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

	};

	class CopyImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageCopy> _regions;

	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> src = nullptr;
			std::shared_ptr<ImageView> dst = nullptr;
			std::vector<VkImageCopy> regions = {};
		};
		using CI = CreateInfo;

		CopyImage(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};

	class CopyBufferToImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<Buffer> _src;
		std::shared_ptr<ImageView> _dst;

	public:

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
		
	};

	class PrepareForPresetation : public DeviceCommand
	{
	protected:

		std::vector<std::shared_ptr<ImageView>> _images = {};
		uint32_t _present_index = 0;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<std::shared_ptr<ImageView>> images = {};
		};

		using CI = CreateInfo;

		PrepareForPresetation(CreateInfo const& ci);

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

		constexpr void setIndex(uint32_t i)
		{
			_present_index = i;
		}

	};
}