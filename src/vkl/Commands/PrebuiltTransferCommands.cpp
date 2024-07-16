#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/Execution/BufferPool.hpp>

namespace vkl
{
	PrebuilTransferCommands::PrebuilTransferCommands(VkApplication * app):
		upload_staging_pool(std::make_shared<BufferPool>(BufferPool::CI{
			.app = app,
			.name = "UploadPool",
			.allocator = app->allocator(),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS,
			.mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		})),
		download_staging_pool(std::make_shared<BufferPool>(BufferPool::CI{
			.app = app,
			.name = "DownloadPool",
			.allocator = app->allocator(),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS,
			.mem_usage = VMA_MEMORY_USAGE_GPU_TO_CPU,
		})),
		copy_image(CopyImage::CI{
			.app = app,
			.name = "CopyImage",
		}),
		copy_buffer_to_image(CopyBufferToImage::CI{
			.app = app,
			.name = "CopyBufferToImage",
		}),
		copy_image_to_buffer(CopyImageToBuffer::CI{
			.app = app,
			.name = "CopyImageToBuffer",
		}),
		copy_buffer(CopyBuffer::CI{
			.app = app,
			.name = "CopyBuffer",
		}),
		fill_buffer(FillBuffer::CI{
			.app = app,
			.name = "FillBuffer",
		}),
		update_buffer(UpdateBuffer::CI{
			.app = app,
			.name = "UpdateBuffer",
		}),
		upload_buffer(UploadBuffer::CI{
			.app = app,
			.name = "UploadBuffer",
			.staging_pool = upload_staging_pool,
		}),
		upload_image(UploadImage::CI{
			.app = app,
			.name = "UploadImage",
			.staging_pool = upload_staging_pool,
		}),
		upload_resources(UploadResources::CI{
			.app = app,
			.name = "UploadResources",
			.staging_pool = upload_staging_pool,
		}),
		download_buffer(DownloadBuffer::CI{
			.app = app,
			.name = "DownloadBuffer",
			.staging_pool = download_staging_pool,
		}),
		download_image(DownloadImage::CI{
			.app = app,
			.name = "DownloadImage",
			.staging_pool = download_staging_pool,
		}),
		blit_image(BlitImage::CI{
			.app = app,
			.name = "BlitImage",
		}),
		compute_mips(ComputeMips::CI{
			.app = app,
			.name = "ComputeMips",
		}),
		clear_image(ClearImage::CI{
			.app = app,
			.name = "ClearImage",
		})
	{}
}