#include <vkl/VkObjects/ImageView.hpp>

#include <vkl/GUI/DescriptorInstancePanel.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>
#include <vkl/GUI/InlinePanel.hpp>

namespace vkl
{
	std::atomic<size_t> ImageViewInstance::_instance_counter = 0;
	
	void ImageViewInstance::create()
	{
		_ci.image = *_image;
		VK_CHECK(vkCreateImageView(_app->device(), &_ci, nullptr, &_view), "Failed to create an image view.");

		setVkNameIFP();
	}

	void ImageViewInstance::setVkNameIFP()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT view_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
				.objectHandle = (uint64_t)_view,
				.pObjectName = name().c_str(),
			};
			_app->nameVkObjectIFP(view_name);
		}
	}

	void ImageViewInstance::destroy()
	{
		assert(!!_view);
		callDestructionCallbacks();
		vkDestroyImageView(_app->device(), _view, nullptr);
		_view = VK_NULL_HANDLE;
		_image = nullptr;
	}

	ImageViewInstance::ImageViewInstance(CreateInfo const& ci):
		AbstractInstance(ci.app, ci.name, ci.tick),
		_image(ci.image),
		_ci(ci.ci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		create();
	}

	ImageViewInstance::~ImageViewInstance()
	{
		if (!!_view)
		{
			destroy();
		}
	}


	void ImageView::createInstance(size_t tick)
	{
		assert(!_inst);
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = _type,
			.format = *_format,
			.components = _components,
			.subresourceRange = *_range,
		};
		
		_inst = std::make_shared<ImageViewInstance>(ImageViewInstance::CI{
			.app = application(),
			.name = name(),
			.tick = tick,
			.image = _image->instance(),
			.ci = ci,
		});
	}

	ImageView::~ImageView()
	{
		_image->removeInvalidationCallback(this);
	}

	void ImageView::constructorBody(bool create_instance)
	{
		_image->setInvalidationCallback(Callback{
			.callback = [&]()
			{
				this->destroyInstanceIFN();
			},
			.id = this,
		});
		if (create_instance)
		{
			createInstance();
		}
	}

	ImageView::ImageView(CreateInfo const& ci) :
		InstanceHolder<ImageViewInstance>((ci.app ? ci.app : ci.image->application()), ci.name, ci.hold_instance),
		_image(ci.image),
		_type(ci.type == VK_IMAGE_TYPE_MAX_ENUM ? getDefaultViewTypeFromImageType(_image->type()) : ci.type),
		_format(ci.format.hasValue() ? ci.format : _image->format()),
		_components(ci.components),
		_range(ci.range.hasValue() ? ci.range : _image->fullSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}

	ImageView::ImageView(Image::CreateInfo const& ci):
		InstanceHolder<ImageViewInstance>(ci.app, ci.name, ci.hold_instance),
		_image(std::make_shared<Image>(ci)),
		_type(getDefaultViewTypeFromImageType(_image->type())),
		_format(_image->format()),
		_components(defaultComponentMapping()),
		_range(_image->fullSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}


	void ImageView::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		res.invalidated = _image->updateResources(ctx).invalidated;

		if (!res.invalidated && _inst)
		{
			res.invalidated = [&]()
			{
				const VkImageViewCreateInfo & inst_ci = _inst->createInfo();
				const VkFormat new_format = *_format;
				if (inst_ci.format != new_format)
				{
					return true;
				}
				const VkImageSubresourceRange range = *_range;
				if (inst_ci.subresourceRange != range)
				{
					return true;
				}
				return false;
			}();

			if (res.invalidated)
			{
				destroyInstanceIFN();
			}
		}

		if (!_inst)
		{
			res.created = true;
			createInstance(ctx.updateTick());
		}
	}

	namespace GUI
	{
		class ImageViewInstanceInspector : public InstanceInspector<ImageViewInstance>
		{
			using Parent = InstanceInspector<ImageViewInstance>;
		protected:

			IndirectInlinePanel _image_panel;

		public:

			ImageViewInstanceInspector(std::shared_ptr<ImageViewInstance> const& target):
				Parent(target)
			{
				_image_panel = IndirectInlinePanel::MakeUniqueIndirectPanel(_target->image());
				_image_panel.child_label = "Image";
			}

			static void DeclareSubresourceRange(Context& ctx, VkImageSubresourceRange const& range)
			{
				InspectVkBitField<VkImageAspectFlagBits>(ctx, "Apsect Mask", range.aspectMask);
				ImGui::LabelValue("Base Mip level", range.baseMipLevel);
				ImGui::LabelValue("Level count", range.levelCount);
				ImGui::LabelValue("Base array layer", range.baseArrayLayer);
				ImGui::LabelValue("Layer count", range.layerCount);
			}

			virtual void declareInline(Context& ctx) override
			{
				const auto& ci = _target->_ci;
				InspectVkBitField<VkImageViewCreateFlagBits>(ctx, "Creation Flags", ci.flags);
				InspectVkEnum(ctx, "View Type", ci.viewType);
				InspectVkEnum(ctx, "Format", ci.format);

				if(ImGui::TreeNode("Components"))
				{
					const VkComponentSwizzle* comp_swizzle = &ci.components.r;
					std::array comp_names = {'R', 'G', 'B', 'A'};
					char label[] = "R";
					for (uint i = 0; i < 4; ++i)
					{
						label[0] = comp_names[i];
						InspectVkEnum(ctx, label, comp_swizzle[i]);
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Subresource Range", ImGuiTreeNodeFlags_DefaultOpen))
				{
					DeclareSubresourceRange(ctx, ci.subresourceRange);
					ImGui::TreePop();
				}

				_image_panel.declareInline(ctx);
			}
		};

		class ImageViewInspector : public DescriptorInspector<ImageView>
		{
			using Parent = DescriptorInspector<ImageView>;
		protected:

			IndirectInlinePanel _image_panel;

		public:

			ImageViewInspector(std::shared_ptr<ImageView> const& target) :
				Parent(target)
			{
				_image_panel = IndirectInlinePanel::MakeUniqueIndirectPanel(_target->image());
				_image_panel.child_label = "Image";
			}

			virtual void declareInline(Context& ctx) override
			{
				_image_panel.declareInline(ctx);
				Parent::declareInstance(ctx);
			}
		};
	}

	std::shared_ptr<GUI::Panel> ImageViewInstance::makeInspector(std::shared_ptr<ImageViewInstance> const& shared_this, GUI::Context& ctx)
	{
		return GUI::MakeInspectorFromTarget(ctx, shared_this);
	}

	std::shared_ptr<GUI::Panel> ImageView::makeInspector(std::shared_ptr<ImageView> const& shared_this, GUI::Context& ctx)
	{
		return GUI::MakeInspectorFromTarget(ctx, shared_this);
	}
}