#include <vkl/VkObjects/ImageView.hpp>

#include <vkl/GUI/DescriptorInstancePanel.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>
#include <vkl/GUI/ImageVisualizer.hpp>

#include <format>

namespace vkl
{
	std::atomic<size_t> ImageViewInstance::_instance_counter = 0;
	
	void ImageViewInstance::create()
	{
		_ci.image = *_image;
		VK_CHECK(vkCreateImageView(_app->device(), &_ci, nullptr, &handle()), "Failed to create an image view.");

		registerName();
	}

	void ImageViewInstance::destroy()
	{
		assert(!!handle());
		callDestructionCallbacks();
		vkDestroyImageView(_app->device(), handle(), nullptr);
		handle() = VK_NULL_HANDLE;
		_image = nullptr;
	}

	ImageViewInstance::ImageViewInstance(CreateInfo const& ci):
		Parent(ci.app, ci.name, ci.tick),
		_image(ci.image),
		_ci(ci.ci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		create();
	}

	ImageViewInstance::ImageViewInstance(std::shared_ptr<ImageInstance> const& image) :
		Parent(image->application(), std::format("{}.View", image->name()), image->creationTick()),
		_image(image),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		VkImageCreateInfo const& image_ci = _image->createInfo();
		_ci = VkImageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = _image->handle(),
			.viewType = getDefaultViewTypeFromImageType(image_ci.imageType),
			.format = image_ci.format,
			.components = defaultComponentMapping(),
			.subresourceRange = _image->defaultSubresourceRange(),
		};
	}

	ImageViewInstance::~ImageViewInstance()
	{
		if (!!handle())
		{
			destroy();
		}
	}


	void ImageView::createInstance(size_t tick)
	{
		assert(!_instance);
		VkImageViewCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = _type,
			.format = *_format,
			.components = _components,
			.subresourceRange = *_range,
		};
		
		_instance = std::make_shared<ImageViewInstance>(ImageViewInstance::CI{
			.app = application(),
			.name = name(),
			.tick = tick,
			.image = _image->instancePtr(),
			.ci = ci,
		});
	}

	ImageView::~ImageView()
	{
		_image->removeInvalidationCallback(this);
	}

	void ImageView::setImageInvalidationCallback()
	{
		_image->setInvalidationCallback(Callback{
			.callback = [&]()
			{
				this->destroyInstanceIFN();
			},
			.id = this,
		});
	}

	void ImageView::constructorBody(bool create_instance)
	{
		setImageInvalidationCallback();
		if (create_instance)
		{
			createInstance();
		}
	}

	ImageView::ImageView(CreateInfo const& ci) :
		Parent((ci.app ? ci.app : ci.image->application()), ci.name, ci.hold_instance),
		_image(ci.image),
		_type(ci.type == VK_IMAGE_TYPE_MAX_ENUM ? getDefaultViewTypeFromImageType(_image->type()) : ci.type),
		_format(ci.format.hasValue() ? ci.format : _image->format()),
		_components(ci.components),
		_range(ci.range.hasValue() ? ci.range : _image->dynFullSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}

	ImageView::ImageView(Image::CreateInfo const& ci):
		Parent(ci.app, ci.name, ci.hold_instance),
		_image(std::make_shared<Image>(ci)),
		_type(getDefaultViewTypeFromImageType(_image->type())),
		_format(_image->format()),
		_components(defaultComponentMapping()),
		_range(_image->dynFullSubresourceRange())
	{
		constructorBody(ci.create_on_construct);
	}

	ImageView::ImageView(std::shared_ptr<ImageViewInstance> const& inst) :
		Parent(inst),
		_image(std::make_shared<Image>(instance()->image())),
		_type(instance()->createInfo().viewType),
		_format(instance()->createInfo().format),
		_components(instance()->createInfo().components),
		_range(instance()->createInfo().subresourceRange)
	{
		// Not necessary since the image is a static descriptor too, TODO think about (maybe make a static descriptor type)
		setImageInvalidationCallback();
	}

	ImageView::ImageView(std::shared_ptr<ImageInstance> const& image_inst) :
		ImageView(std::make_shared<ImageViewInstance>(image_inst))
	{}



	void ImageView::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		res.invalidated = _image->updateResources(ctx).invalidated;

		if (!res.invalidated && _instance)
		{
			res.invalidated = [&]()
			{
				const VkImageViewCreateInfo & inst_ci = instance()->createInfo();
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

		if (!_instance)
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

			ObjectInlineInspector _image_panel;

			ImageVisualizer _visualizer;

		public:

			ImageViewInstanceInspector(std::shared_ptr<ImageViewInstance> const& target, Context& ctx):
				Parent(target),
				_image_panel("Image"),
				_visualizer(ImageVisualizer::CI{
					.ctx = &ctx,
					.label = "Visualizer",
				})
			{
				_image_panel.setAcceptNullptr(false);
				_image_panel.setDisableCreation(true);
				_image_panel.setHideCreateRemoveButton(true);
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
				const auto& ci = target()->_ci;
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

				_image_panel.declareInline(ctx, target()->image());

				SectionBox visu_section{};
				visu_section.label = _visualizer.label().data();
				if (visu_section.begin(ctx))
				{
					_visualizer.setSource(targetPtr());
					_visualizer.declareInline(ctx);
				}
				visu_section.end(ctx);
			}
		};

		class ImageViewInspector : public DescriptorInspector<ImageView>
		{
			using Parent = DescriptorInspector<ImageView>;
		protected:

			ObjectInlineInspector _image_panel;

		public:

			ImageViewInspector(std::shared_ptr<ImageView> const& target) :
				Parent(target),
				_image_panel("Image")
			{
				_image_panel.setAcceptNullptr(false);
				_image_panel.setDisableCreation(true);
				_image_panel.setHideCreateRemoveButton(true);
			}

			virtual void declareInline(Context& ctx) override
			{
				_image_panel.declareInline(ctx, target()->image());
				Parent::declareInstance(ctx);
			}
		};
	}

	std::shared_ptr<GUI::Panel> ImageViewInstance::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return GUI::MakeInspectorFromTarget(ctx, std::static_pointer_cast<ImageViewInstance>(shared_this));
	}

	std::shared_ptr<GUI::Panel> ImageView::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return GUI::MakeInspectorFromTarget(ctx, std::static_pointer_cast<ImageView>(shared_this));
	}
}