#include "Framebuffer.hpp"
#include <cassert>

namespace vkl
{
	FramebufferInstance::FramebufferInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_textures(ci.targets),
		_depth_stencil(ci.depth_stencil),
		_render_pass(ci.render_pass),
		_layers(ci.layers)
	{
		std::vector<VkImageView> views(_textures.size());
		for (size_t i = 0; i < _textures.size(); ++i)	views[i] = *_textures[i];
		if (_depth_stencil)
		{
			views.push_back(*_depth_stencil);
		}
		VkExtent3D _extent = extent();
		VkFramebufferCreateInfo vk_ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = *_render_pass,
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = _extent.width,
			.height = _extent.height,
			.layers = _layers,
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
		_render_pass(ci.render_pass),
		_layers(ci.layers)
	{
		Callback cb{
			.callback = [&]() {
				destroyInstance();
			},
			.id = this,
		};
		_render_pass->addInvalidationCallback(cb);

		if (!_layers.hasValue())
		{
			_layers = [this]() {
				uint32_t res = uint32_t(-1);
				for (size_t i = 0; i < _textures.size(); ++i)
				{
					res = std::min(res, _textures[i]->range().value().layerCount);
				}
				if (_depth_stencil)
				{
					res = std::min(res, _depth_stencil->range().value().layerCount);
				}
				return res;
			};
		}

		for (size_t i = 0; i < _textures.size(); ++i)
		{
			_textures[i]->addInvalidationCallback(cb);
		}
		if (_depth_stencil)
		{
			_depth_stencil->addInvalidationCallback(cb);
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
		if (_depth_stencil)
		{
			_depth_stencil->removeInvalidationCallbacks(this);
		}
	}

	bool Framebuffer::updateResources(UpdateContext & ctx)
	{
		bool res = false;

		if (_inst)
		{
			const uint32_t new_layers = _layers.value();
			if (new_layers != _inst->layers())
			{
				res = true;
			}

			if (res)
			{
				destroyInstance();
			}
		}

		if (!_inst)
		{
			createInstance();
		}

		return res;
	}
}