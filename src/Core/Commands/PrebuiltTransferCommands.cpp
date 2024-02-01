#include "PrebuiltTransferCommands.hpp"

namespace vkl
{
	PrebuilTransferCommands::PrebuilTransferCommands(VkApplication * app):
		copy_image(CopyImage::CI{
			.app = app,
			.name = "CopyImage",
		}),
		copy_buffer_to_image(CopyBufferToImage::CI{
			.app = app,
			.name = "CopyBufferToImage",
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
		}),
		upload_image(UploadImage::CI{
			.app = app,
			.name = "UploadImage",
		}),
		upload_resources(UploadResources::CI{
			.app = app,
			.name = "UploadResources",
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