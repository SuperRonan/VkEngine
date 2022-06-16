#include "Framebuffer.hpp"
#include <cassert>

namespace vkl
{
	Framebuffer::Framebuffer(std::vector<std::shared_ptr<ImageView>>&& textures, VkRenderPass render_pass) :
		VkObject(textures.front()->application()),
		_textures(std::move(textures)),
		_render_pass(render_pass)
	{
		assert(!_textures.empty());
		std::vector<VkImageView> views(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)	views[i] = *_textures[i];
		VkFramebufferCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = _render_pass,
			.attachmentCount = (uint32_t)views.size(),
			.pAttachments = views.data(),
			.width = _textures.front()->image()->extent().width,
			.height = _textures.front()->image()->extent().height,
			.layers = 1,
		};

		VK_CHECK(vkCreateFramebuffer(_app->device(), &ci, nullptr, &_handle), "Failed to create a Framebuffer.");
	}

	Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
	{
		VkObject::operator=(std::move(other));
		std::swap(_textures, other._textures);
		std::swap(_render_pass, other._render_pass);
		std::swap(_handle, other._handle);
		return *this;
	}

	Framebuffer::~Framebuffer()
	{
		if (handle())
		{
			destroy();
		}
	}

	void Framebuffer::destroy()
	{
		vkDestroyFramebuffer(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
		_render_pass = VK_NULL_HANDLE;
	}
}