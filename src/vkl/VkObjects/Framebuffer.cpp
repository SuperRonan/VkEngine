#include <vkl/VkObjects/Framebuffer.hpp>
#include <cassert>

namespace vkl
{
	FramebufferInstance::FramebufferInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_attachments(ci.attachments),
		_render_pass(ci.render_pass),
		_extent(ci.extent)
	{
		if (ci.render_pass && ci.render_pass->handle())
		{
			MyVector<VkImageView> attachments(_attachments.size());
			for (size_t i = 0; i < attachments.size(); ++i)	attachments[i] = *_attachments[i];
			VkFramebufferCreateInfo vk_ci = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = *_render_pass,
				.attachmentCount = attachments.size32(),
				.pAttachments = attachments.data(),
				.width = _extent.width,
				.height = _extent.height,
				.layers = _extent.depth,
			};

			VK_CHECK(vkCreateFramebuffer(device(), &vk_ci, nullptr, &_handle), "Failed to create a Framebuffer.");
		}
	}

	FramebufferInstance::~FramebufferInstance()
	{
		callDestructionCallbacks();
		if (_handle)
		{
			vkDestroyFramebuffer(_app->device(), _handle, nullptr);
			_handle = VK_NULL_HANDLE;
		}
		_render_pass = nullptr;
		_attachments.clear();
	}



	void Framebuffer::createInstanceIFP()
	{
		assert(!_inst);

		FramebufferInstance::CreateInfo ci{
			.app = application(),
			.name = name(),
			.render_pass = _render_pass->instance(),
		};
		ci.attachments.resize(_attachments.size());
		bool create = _render_pass->instance().operator bool();
		int set_extent = 2;
		if (_extent.hasValue())
		{
			ci.extent = _extent.value();
			set_extent = 0;
		}
		else
		{
			ci.extent = makeUniformExtent3D(uint32_t(-1));
		}
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			ci.attachments[i] = _attachments[i]->instance();
			if (ci.attachments[i])
			{
				create = true;
				if (set_extent)
				{
					VkExtent3D image_extent = ci.attachments[i]->image()->createInfo().extent;
					image_extent.depth = ci.attachments[i]->createInfo().subresourceRange.layerCount;
					ci.extent = minimumExtent(ci.extent, image_extent);
					set_extent = 1;
				}
			}
		}
		if (set_extent == 2)
		{
			ci.extent = makeUniformExtent3D(0);
		}
		if (create)
		{
			_inst = std::make_shared<FramebufferInstance>(ci);
		}
	}

	Framebuffer::Framebuffer(CreateInfo const& ci):
		InstanceHolder<FramebufferInstance>(ci.app, ci.name, ci.hold_instance),
		_attachments(ci.attachments),
		_extent(ci.extent),
		_render_pass(ci.render_pass)
	{
		assert(_render_pass);
		Callback cb{
			.callback = [&]() {
				destroyInstanceIFN();
			},
			.id = this,
		};
		_render_pass->setInvalidationCallback(cb);

		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			_attachments[i]->setInvalidationCallback(cb);
		}
	}

	Framebuffer::~Framebuffer()
	{
		_render_pass->removeInvalidationCallback(this);
		for (size_t i = 0; i < _attachments.size(); ++i)
		{
			_attachments[i]->removeInvalidationCallback(this);
		}
		_attachments.clear();
	}

	bool Framebuffer::updateResources(UpdateContext & ctx)
	{
		bool res = false;
		
		if (checkHoldInstance())
		{
			if (_inst)
			{
				if (_extent)
				{
					const VkExtent3D new_extent = *_extent;
					if (new_extent != _inst->extent())
					{
						res = true;
					}
				}

				if (res)
				{
					destroyInstanceIFN();
				}
			}

			if (!_inst)
			{
				createInstanceIFP();
			}
		}

		return res;
	}
}