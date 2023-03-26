#include "Framebuffer.hpp"
#include <cassert>

namespace vkl
{
	Framebuffer::Framebuffer(CreateInfo const& ci) :
		VkObject(ci.render_pass->application(), ci.name),
		_textures(ci.targets),
		_depth(ci.depth),
		_render_pass(ci.render_pass)
	{
		assert(!_textures.empty());
		std::vector<VkImageView> views(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)	views[i] = *_textures[i]->instance();
		if (_depth)
		{
			views.push_back(*_depth->instance());
		}
		VkFramebufferCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = *_render_pass,
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = _textures.front()->image()->extent().value().width,
			.height = _textures.front()->image()->extent().value().height,
			.layers = 1,
		};

		VK_CHECK(vkCreateFramebuffer(device(), &vk_ci, nullptr, &_handle), "Failed to create a Framebuffer.");
	}

	Framebuffer::Framebuffer(Framebuffer&& other) noexcept :
		VkObject(std::move(other)),
		_textures(std::move(other._textures)),
		_depth(std::move(other._depth)),
		_render_pass(std::move(other._render_pass)),
		_handle(other._handle)
	{
		other._render_pass = nullptr;
		other._handle = VK_NULL_HANDLE;
	}

	Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
	{
		VkObject::operator=(std::move(other));
		std::swap(_textures, other._textures);
		std::swap(_depth, other._depth);
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
		_render_pass = nullptr;
	}
}