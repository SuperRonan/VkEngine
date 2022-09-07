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

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

	};

	class CopyImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageCopy> _regions;


	public:

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
}