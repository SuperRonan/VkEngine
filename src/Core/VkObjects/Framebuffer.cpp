#include "Framebuffer.hpp"
#include <cassert>

namespace vkl
{
	FramebufferInstance::FramebufferInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_textures(ci.targets),
		_depth_stencil(ci.depth_stencil),
		_render_pass(ci.render_pass)
	{
		std::vector<VkImageView> views(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)	views[i] = *_textures[i];
		if (_depth_stencil)
		{
			views.push_back(*_depth_stencil);
		}
		VkFramebufferCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = *_render_pass,
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = extent().width,
			.height = extent().height,
			.layers = 1,
		};

		VK_CHECK(vkCreateFramebuffer(device(), &vk_ci, nullptr, &_handle), "Failed to create a Framebuffer.");
	}

	FramebufferInstance::~FramebufferInstance()
	{
		callDestructionCallbacks();
		vkDestroyFramebuffer(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
		_render_pass = nullptr;
	}



	void Framebuffer::createInstance()
	{
		if (_inst)
		{
			destroyInstance();
		}
		FramebufferInstance::CreateInfo ci{
			.app = application(),
			.name = name(),
			.render_pass = _render_pass->instance(),
			.depth_stencil = _depth_stencil ? _depth_stencil->instance() : nullptr,
		};
		ci.targets.resize(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)
		{
			ci.targets[i] = _textures[i]->instance();
		}
		_inst = std::make_shared<FramebufferInstance>(ci);
	}


	void Framebuffer::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	Framebuffer::Framebuffer(CreateInfo const& ci):
		InstanceHolder<FramebufferInstance>(ci.app, ci.name),
		_textures(ci.targets),
		_depth_stencil(ci.depth_stencil),
		_render_pass(ci.render_pass)
	{
		Callback cb{
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		};
		_render_pass->addInvalidationCallback(cb);
		for (size_t i = 0; i < _textures.size(); ++i)
		{
			_textures[i]->addInvalidationCallback(cb);
		}
	}

	Framebuffer::~Framebuffer()
	{
		destroyInstance();
		_render_pass->removeInvalidationCallbacks(this);
		for (size_t i = 0; i < _textures.size(); ++i)
		{
			_textures[i]->removeInvalidationCallbacks(this);
		}
	}

	bool Framebuffer::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		if (!_inst)
		{
			createInstance();
		}

		return res;
	}
}