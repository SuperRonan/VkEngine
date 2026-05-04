#include <vkl/GUI/ImageVisualizer.hpp>

#include <vkl/GUI/Context.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <vulkan/utility/vk_format_utils.h>

#include <vkl/VkObjects/VulkanEnumMeta.hpp>

namespace vkl::GUI
{
	ImageVisualizer::ImageVisualizer(CreateInfo const& ci) :
		_label(ci.label),
		_sampler(ci.ctx->sampler())
	{
		createSet(ci.ctx->getImGuiSetLayout());
	}

	ImageVisualizer::~ImageVisualizer()
	{

	}

	void ImageVisualizer::setSource(SourceSPtr const& source)
	{
		if (source == _source)
		{
			return;
		}
		clear();
		if (!source)
		{
			_source.reset();
		}
		else if (CheckSourceType(source.getRaw()))
		{
			_source = source;
			createDefaultView();
		}
	}

	void ImageVisualizer::clear(bool keep_error_message)
	{
		if (_set)
		{
			std::shared_ptr<ImageView> view = {};
			_set->setBinding(0, 0, 1, &view, nullptr);
		}
		_custom_view.reset();
		_custom_view_desc.reset();
		if(!keep_error_message)
			_error_message.clear();
	}

	void ImageVisualizer::createSet(std::shared_ptr<DescriptorSetLayoutInstance> const& layout)
	{
		_set = std::make_shared<DescriptorSetAndPoolInstance>(DescriptorSetAndPoolInstance::CI{
			.app = layout->application(),
			.name = _label + ".Set",
			.layout = layout,
		});
	}

	void ImageVisualizer::createDefaultView()
	{
		if (!_source)
		{
			_error_message = "No source to visualize.";
			return;
		}
		Image* source_image = dynamic_cast<Image*>(_source.getRaw());
		ImageInstance* source_image_instance = dynamic_cast<ImageInstance*>(_source.getRaw());
		ImageView* source_image_view = dynamic_cast<ImageView*>(_source.getRaw());
		ImageViewInstance* source_image_view_instance = dynamic_cast<ImageViewInstance*>(_source.getRaw());

		std::shared_ptr<ImageInstance> image_instance;
		std::shared_ptr<ImageViewInstance> image_view_instance;

		const std::string_view empty_descriptor_error = "Descriptor does not have an Instance.";

		if (source_image)
		{
			image_instance = source_image->instancePtr();
			if (!image_instance)
			{
				_error_message = empty_descriptor_error;
				return clear(true);
			}
		}
		else if (source_image_instance)
		{
			image_instance = std::static_pointer_cast<ImageInstance>(_source.get());
		}
		else
		{
			if (source_image_view)
			{
				image_view_instance = source_image_view->instancePtr();
				if (!image_view_instance)
				{
					_error_message = empty_descriptor_error;
					return clear(true);
				}
			}
			else if (source_image_view_instance)
			{
				image_view_instance = std::static_pointer_cast<ImageViewInstance>(_source.get());
			}
			image_instance = image_view_instance->image();
		}
		VkImageCreateInfo const& image_ci = image_instance->createInfo();
		const VkImageViewCreateInfo* view_ci = nullptr;
		if (image_view_instance)
		{
			view_ci = &image_view_instance->createInfo();
		}
		if (image_ci.imageType != VK_IMAGE_TYPE_2D)
		{
			_error_message = std::format(
				"Invalid Image Type ({}).\n"
				"Expected {}.",
				vku::GetEnumLabel(image_ci.imageType), vku::GetEnumLabel(VK_IMAGE_TYPE_2D));
			return clear(true);
		}

		if (!(image_ci.usage & VK_IMAGE_USAGE_SAMPLED_BIT))
		{
			_error_message = std::format(
				"Image does not support sampling."
			);
			return clear(true);
		}

		if (view_ci)
		{
			_custom_format = view_ci->format;
			_custom_aspect = view_ci->subresourceRange.aspectMask;
			_custom_mips_range = Range32u(view_ci->subresourceRange.baseMipLevel, view_ci->subresourceRange.levelCount);
			_array_layer = view_ci->subresourceRange.baseArrayLayer;
			_custom_swizzle = view_ci->components;
		}
		else
		{
			_custom_format = image_ci.format;
			_custom_aspect = getImageAspectFromFormat(_custom_format);
			_custom_mips_range = Range32u(0, image_ci.mipLevels);
			_array_layer = 0;
			_custom_swizzle = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_ONE,
			};
		}
		// Better default choice
		_custom_swizzle.a = VK_COMPONENT_SWIZZLE_ONE;

		_uv_tl = Vector2f(0, 0);
		_uv_br = Vector2f(1, 1);
		_size_pix = Vector2f(image_ci.extent.width, image_ci.extent.height);

		createCurstomView(image_instance);
	}

	void ImageVisualizer::createCurstomView(std::shared_ptr<ImageInstance> const& image)
	{
		VkImageViewCreateInfo custom_view_ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = image->handle(),
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = _custom_format,
			.components = _custom_swizzle,
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = _custom_aspect,
				.baseMipLevel = _custom_mips_range.begin,
				.levelCount = _custom_mips_range.len,
				.baseArrayLayer = _array_layer,
				.layerCount = 1,
			},
		};

		_custom_view = std::make_shared<ImageViewInstance>(ImageViewInstance::CI{
			.app = image->application(),
			.name = std::format("{}.CustomView", _label),
			.image = image,
			.ci = custom_view_ci,
		});

		_custom_view_desc = std::make_shared<ImageView>(_custom_view);

		_set->setBinding(0, 0, 1, &_custom_view_desc, &_sampler);
		_set->writeDescriptorSet();
	}

	static inline ImVec2 ToIMGui(Vector2f const& v)
	{
		return ImVec2(v.x(), v.y());
	}

	static inline ImVec4 ToIMGui(Vector4f const& v)
	{
		return ImVec4(v.x(), v.y(), v.z(), v.w());
	}

	void ImageVisualizer::declareImage(Context& ctx, ImVec2 const& size, const ImRect* rect, bool skip_registration)
	{
		ImGui::ImageWithBg(_set->set()->handle(), size, rect ? rect->GetTL() : ImVec2(0, 0), rect ? rect->GetBR() : ImVec2(1, 1), ToIMGui(_background), ToIMGui(_tint));
		//ImGui::GetWindowDrawList()->AddCallback()
		if (!skip_registration)
		{
			ctx.addFrameImage(_custom_view);
			ctx.keepFrameObject(_set);
		}
	}

	void ImageVisualizer::declareInline(Context& ctx)
	{
		if (_custom_view)
		{
			ImRect rect(ToIMGui(_uv_tl), ToIMGui(_uv_br));
			declareImage(ctx, ToIMGui(_size_pix), &rect);
		}
		else
		{
			ImVec4 col = ctx.style().invalid_red;
			ImGui::TextColored(col, "Cannot visualize image:");
			const char* reason = _error_message.empty() ? "Unknown reason." : _error_message.c_str();
			ImGui::TextColored(col, reason);
		}
	}
}