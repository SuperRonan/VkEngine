#include "Framebuffer.hpp"
#include <cassert>

namespace vkl
{
	Framebuffer::Framebuffer(std::vector<std::shared_ptr<ImageView>>&& textures) :
		VkObject(textures.front()->application()),
		_textures(std::move(textures))
	{
		assert(!_textures.empty());
		std::vector<VkImageView> views(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)	views[i] = *_textures[i];
		VkFramebufferCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = (uint32_t)views.size(),
			.pAttachments = views.data(),
			//.width = _textures.front()->image()
		};
	}

	void Framebuffer::destroy()
	{
		vkDestroyFramebuffer(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}
}