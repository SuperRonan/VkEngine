#pragma once

#include "TransferCommand.hpp"
#include "GraphicsTransferCommands.hpp"

namespace vkl
{
	// Works because stateless commands
	struct PrebuilTransferCommands
	{
		PrebuilTransferCommands(VkApplication * app);

		std::shared_ptr<BufferPool> upload_staging_pool;
		std::shared_ptr<BufferPool> download_staging_pool;

		CopyImage copy_image;
		CopyBufferToImage copy_buffer_to_image;
		CopyImageToBuffer copy_image_to_buffer;
		CopyBuffer copy_buffer;
		FillBuffer fill_buffer;
		UpdateBuffer update_buffer;
		UploadBuffer upload_buffer;
		UploadImage upload_image;
		UploadResources upload_resources;

		BlitImage blit_image;
		ComputeMips compute_mips;
		ClearImage clear_image;
	};
}