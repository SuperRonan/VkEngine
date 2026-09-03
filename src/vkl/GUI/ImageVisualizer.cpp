#define IMGUI_DEFINE_MATH_OPERATORS 1
#include <vkl/GUI/ImageVisualizer.hpp>

#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/FancyButtons.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <vulkan/utility/vk_format_utils.h>

#include <vkl/VkObjects/VulkanEnumMeta.hpp>

#include <algorithm>

namespace vkl::GUI
{
	ImageVisualizer::ImageVisualizer(CreateInfo const& ci) :
		_label(ci.label),
		_sampler_panel("Sampler")
	{
		createTextureSet(ci.ctx->getImGuiTextureSetLayout());

		_sampler_panel.setAcceptNullptr();
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
		if (_texture_set)
		{
			std::shared_ptr<ImageView> view = {};
			_texture_set->setBinding(0, 0, 1, &view, nullptr);
		}
		_custom_view.reset();
		_custom_view_desc.reset();
		if(!keep_error_message)
			_error_message.clear();
	}

	void CreateSet(std::shared_ptr<DescriptorSetAndPoolInstance>& set, std::shared_ptr<DescriptorSetLayoutInstance> const& layout, std::string_view name)
	{
		set = std::make_shared<DescriptorSetAndPoolInstance>(DescriptorSetAndPoolInstance::CI{
			.app = layout->application(),
			.name = std::string(name),
			.layout = layout,
		});
	}

	void ImageVisualizer::createTextureSet(std::shared_ptr<DescriptorSetLayoutInstance> const& layout)
	{
		CreateSet(_texture_set, layout, _label + ".TextureSet");
	}

	void ImageVisualizer::createSamplerSet(std::shared_ptr<DescriptorSetLayoutInstance> const& layout)
	{
		CreateSet(_sampler_set, layout, _label + ".SamplerSet");
	}

	static inline VkComponentMapping GetDefaultMapping(VkFormat f)
	{
		VKU_FORMAT_INFO info = vkuGetFormatInfo(f);
		VkComponentMapping res = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_ONE,
		};
		VkComponentSwizzle* swizzle = &res.r;
		for (uint i = 0; i < info.component_count; ++i)
		{
			swizzle[i] = VK_COMPONENT_SWIZZLE_IDENTITY;
		}
		return res;
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
			_latest_source_instance = image_instance;
		}
		else if (source_image_instance)
		{
			image_instance = std::static_pointer_cast<ImageInstance>(_source.get());
			_latest_source_instance = image_instance;
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
				_latest_source_instance = image_view_instance;
			}
			else if (source_image_view_instance)
			{
				image_view_instance = std::static_pointer_cast<ImageViewInstance>(_source.get());
				_latest_source_instance = image_view_instance;
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

		VkComponentMapping custom_swizzle;
		Range32u array_range = {}, allowed_mips_range = {};

		if (view_ci)
		{
			_custom_format = view_ci->format;
			_custom_aspect = view_ci->subresourceRange.aspectMask;
			auto view_range = image_view_instance->finiteRange();
			allowed_mips_range = Range32u{.begin = view_range.baseMipLevel, .len = view_range.levelCount};
			array_range = Range32u{.begin = view_range.baseArrayLayer, .len = view_range.layerCount};
			custom_swizzle = view_ci->components;
		}
		else
		{
			_custom_format = image_ci.format;
			_custom_aspect = getImageAspectFromFormat(_custom_format);
			_custom_mips_range = Range32u(0, image_ci.mipLevels);
			array_range = Range32u{.begin = 0, .len = image_instance->createInfo().arrayLayers};
			allowed_mips_range = Range32u{.begin = 0, .len = image_instance->createInfo().mipLevels};
			custom_swizzle = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_ONE,
			};
		}
		// Better default choice
		custom_swizzle.a = VK_COMPONENT_SWIZZLE_ONE;
		if (_custom_aspect & VK_IMAGE_ASPECT_DEPTH_BIT)
		{
			custom_swizzle = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_R,
				.b = VK_COMPONENT_SWIZZLE_R,
				.a = VK_COMPONENT_SWIZZLE_ONE,
			};
		}
		if (_manual_swizzle)
		{
			custom_swizzle = _custom_swizzle;
		}
		_custom_swizzle = custom_swizzle;

		if (_manual_array_layer)
		{
			_array_layer = array_range.clamp(_array_layer);
		}
		else
		{
			_array_layer = array_range.begin;
		}

		if (_manual_mips_range)
		{
			Range32u old_range = _custom_mips_range;
			_custom_mips_range.begin = allowed_mips_range.clamp(old_range.begin);
			if (old_range.len != Range32u::NPos)
			{
				uint32_t allowed_new_len = allowed_mips_range.end() - _custom_mips_range.begin;
				_custom_mips_range.len = std::min(allowed_new_len, old_range.len);
			}
		}
		else
		{
			_custom_mips_range = allowed_mips_range;
		}

		if (_size_mode == SizeMode::ImageSize)
		{
			_size_pix = Vector2f(image_ci.extent.width, image_ci.extent.height);
		}

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
		_texture_set->setBinding(0, 0, 1, &_custom_view_desc, nullptr);
		_texture_set->writeDescriptorSet();
	}

	void ImageVisualizer::checkInstance(Context& ctx)
	{
		if (_custom_view)
		{
			assert(_source);
			if (AbstractInstanceHolder * holder = dynamic_cast<AbstractInstanceHolder*>(_source.getRaw()))
			{
				if (holder->instance() != _latest_source_instance.getRaw())
				{
					clear();
				}
			}
		}
		if (!_custom_view)
		{
			createDefaultView();
		}

		if (_sampler)
		{
			if (!_sampler_set)
			{
				createSamplerSet(ctx.getImGuiSamplerSetLayout());
			}
			if (_sampler_set->bindings()[0].images_samplers[0].sampler != _sampler)
			{
				_sampler_set->setBinding(0, 0, 1, nullptr, &_sampler);
				_sampler_set->writeDescriptorSet();
			}
		}
	}

	ImageInstance* ImageVisualizer::GetImageInstance(InstancePtr ptr)
	{
		ImageInstance* ii = dynamic_cast<ImageInstance*>(ptr.get());
		if (!ii)
		{
			ii = static_cast<ImageViewInstance*>(ptr.get())->image().get();
		}
		return ii;
	}

	void ImageVisualizer::calcDisplaySize(Vector2f image_size, float available_width)
	{
		if (std::IsAnyOf(_size_mode, SizeMode::ImageSize))
		{
			_size_pix = image_size;
		}
		else if (std::IsAnyOf(_size_mode, SizeMode::FitWidth, SizeMode::FitWidthIntegral))
		{
			float p = available_width / image_size.x();
			float q = image_size.x() / available_width;
			if (_size_mode == SizeMode::FitWidthIntegral)
			{
				if (image_size.x() > available_width) // downsize
				{
					q = std::ceil(q);
					_size_pix = image_size / q;
					_size_pix = _size_pix.array().floor();
				}
				else // upsize
				{
					p = std::floor(p);
					_size_pix = image_size * p;
				}
			}
			else
			{
				_size_pix = image_size * p;
			}
		}
		else if (_size_mode == SizeMode::Manual && !_manual_unlock_ratio)
		{
			float r = _size_pix.x() / image_size.x();
			_size_pix.y() = image_size.y() * r;
		}
	}

	void ImageVisualizer::calcDisplaySize(float available_width)
	{
		assert(_latest_source_instance);
		ImageInstance* ii = GetImageInstance(_latest_source_instance.getRawVariant());
		Vector2f image_size = Vector2f(ii->createInfo().extent.width, ii->createInfo().extent.height);
		calcDisplaySize(image_size, available_width);
	}

	static inline ImVec2 ToIMGui(Vector2f const& v)
	{
		return ImVec2(v.x(), v.y());
	}

	static inline ImVec4 ToIMGui(Vector4f const& v)
	{
		return ImVec4(v.x(), v.y(), v.z(), v.w());
	}

	static constexpr uint MAX_COMPONENTS = 4;

	static constexpr char SwizzleToChar(VkComponentSwizzle s)
	{
		char res = '?';
		switch (s)
		{
			case VK_COMPONENT_SWIZZLE_IDENTITY: res = 'i'; break;
			case VK_COMPONENT_SWIZZLE_ZERO: res = '0'; break;
			case VK_COMPONENT_SWIZZLE_ONE: res = '1'; break;
			case VK_COMPONENT_SWIZZLE_R: res = 'r'; break;
			case VK_COMPONENT_SWIZZLE_G: res = 'g'; break;
			case VK_COMPONENT_SWIZZLE_B: res = 'b'; break;
			case VK_COMPONENT_SWIZZLE_A: res = 'a'; break;
		}
		return res;
	}

	static constexpr VkComponentSwizzle CharToSwizzle(char c)
	{
		VkComponentSwizzle res = VK_COMPONENT_SWIZZLE_MAX_ENUM;
		if (c >= 'A' && c <= 'Z')
		{
			c += ('a' - 'A');
		}
		switch (c)
		{
			case 'i':
				res = VK_COMPONENT_SWIZZLE_IDENTITY;
			break;
			case '0':
				res = VK_COMPONENT_SWIZZLE_ZERO;
			break;
			case '1':
				res = VK_COMPONENT_SWIZZLE_ONE;
			break;
			case 'r':
			case 'x':
				res = VK_COMPONENT_SWIZZLE_R;
			break;
			case 'g':
			case 'y':
				res = VK_COMPONENT_SWIZZLE_G;
			break;
			case 'b':
			case 'z':
				res = VK_COMPONENT_SWIZZLE_B;
			break;
			case 'a':
			case 'w':
				res = VK_COMPONENT_SWIZZLE_A;
			break;
		}
		return res;
	}

	static constexpr void SwizzleMappingToChars(char* dst, VkComponentMapping const& m)
	{
		const VkComponentSwizzle* s = &m.r;
		for (uint i = 0; i < MAX_COMPONENTS; ++i)
		{
			dst[i] = SwizzleToChar(s[i]);
		}
	}

	static constexpr bool SwizzleIsValid(VkComponentSwizzle s)
	{
		return s <= VK_COMPONENT_SWIZZLE_A;
	}

	static constexpr bool MappingIsValid(VkComponentMapping const& m)
	{
		const VkComponentSwizzle * s = &m.r;
		return std::all_of(s, s + MAX_COMPONENTS, SwizzleIsValid);
	}

	static inline bool InspectSwizzleMapping(Context& ctx, VkComponentMapping* swizzle)
	{
		assert(swizzle);
		std::array<char, MAX_COMPONENTS + 1> chars;
		VkComponentSwizzle* comp_swizzle = &swizzle->r;
		chars.back() = 0;
		for (uint i = 0; i < MAX_COMPONENTS; ++i)
		{
			chars[i] = SwizzleToChar(comp_swizzle[i]);
		}
		bool res = false;
		if (ImGui::InputText("Swizzle", chars.data(), chars.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AlwaysOverwrite))
		{
			for (uint i = 0; i < MAX_COMPONENTS; ++i)
			{
				VkComponentSwizzle x = CharToSwizzle(chars[i]);
				if (!SwizzleIsValid(x))
				{
					x = VK_COMPONENT_SWIZZLE_IDENTITY;
				}
				if (x != comp_swizzle[i])
				{
					comp_swizzle[i] = x;
					res = true;
				}
			}
		}
		return res;
	}

	static void ImGui_ImplVulkan_DrawCallback_SetSampler(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
		ImGui_ImplVulkan_RenderState* rs = static_cast<ImGui_ImplVulkan_RenderState*>(pio.Renderer_RenderState);
		VkDescriptorSet ds = static_cast<VkDescriptorSet>(cmd->UserCallbackData);
		vkCmdBindDescriptorSets(rs->CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rs->PipelineLayout, 1, 1, &ds, 0, nullptr);
	}

	bool ImageVisualizer::declareImage(Context& ctx, ImVec2 const& size, const ImRect* rect, bool skip_registration)
	{
		if (!skip_registration)
		{
			checkInstance(ctx);
		}
		if (_custom_view)
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
			bool should_restore_sampler = false;
			// TODO use an inplace vector in C++26
			std::array<std::shared_ptr<VkObject>, 4> objects_to_keep = {};
			uint32_t objects_to_keep_count = 0;
			if (_sampler)
			{
				draw_list->AddCallback(ImGui_ImplVulkan_DrawCallback_SetSampler, _sampler_set->set()->handle());
				if (!skip_registration)
				{
					objects_to_keep[objects_to_keep_count + 0] = _sampler_set;
					objects_to_keep[objects_to_keep_count + 1] = _sampler;
					objects_to_keep_count += 2;
				}
				should_restore_sampler = true;
			}
			else if (_imgui_sampler != ImGuiSampler::Default)
			{
				//pio.Renderer_RenderState
				ImDrawCallback cb = _imgui_sampler == ImGuiSampler::Linear ? pio.DrawCallback_SetSamplerLinear : pio.DrawCallback_SetSamplerNearest;
				draw_list->AddCallback(cb);
				should_restore_sampler = true;
			}
			ImGui::ImageWithBg(_texture_set->set()->handle(), size, rect ? rect->GetTL() : ImVec2(0, 0), rect ? rect->GetBR() : ImVec2(1, 1), ToIMGui(_background), ToIMGui(_tint));
			if (should_restore_sampler)
			{
				Context::BoundSampler bound_sampler = ctx.getImGuiBoundSampler();
				if (bound_sampler.ptr)
				{
					draw_list->AddCallback(ImGui_ImplVulkan_DrawCallback_SetSampler, bound_sampler.set);
				}
				else
				{
					draw_list->AddCallback(pio.DrawCallback_ResetRenderState);
				}
			}

			if (!skip_registration)
			{
				ImGuiViewport* viewport = ImGui::GetWindowViewport();
				ctx.addFrameImage(_custom_view);
				objects_to_keep[objects_to_keep_count + 0] = _custom_view;
				objects_to_keep[objects_to_keep_count + 1] = _texture_set;
				objects_to_keep_count += 2;
			}
			if (objects_to_keep_count)
			{
				ctx.keepFrameObjectsMove(std::span(objects_to_keep.data(), objects_to_keep_count));
			}
			return true;
		}
		return false;
	}

	static inline void CalcLayersRange(ImageVisualizer::InstancePtr image, Range32i* p_layers_range, Range32i* p_mips_range)
	{
		assert(image);
		if (p_layers_range)
		{
			*p_layers_range = Range32i{ .begin = 0, .len = 1 };
		}
		if (p_mips_range)
		{
			*p_mips_range = Range32i{ .begin = 0, .len = 1 };
		}
		image.visit(std::overloads{
			[&](ImageInstance const& image) {
				if (p_layers_range)
				{
					p_layers_range->len = image.createInfo().arrayLayers;
				}
				if (p_mips_range)
				{
					p_mips_range->len = image.createInfo().mipLevels;
				}
			},
			[&](ImageViewInstance const& view) {
				auto range = view.createInfo().subresourceRange;
				auto finite_range = view.finiteRange();
				if (p_layers_range)
				{
					p_layers_range->begin = range.baseArrayLayer;
					p_layers_range->len = finite_range.layerCount;
				}
				if (p_mips_range)
				{
					p_mips_range->begin = range.baseMipLevel;
					p_mips_range->len = finite_range.levelCount;
				}
			},
		});
	}

	ImageVisualizer::Result ImageVisualizer::declareInline(Context& ctx)
	{
		float available_width = ImGui::GetCurrentWindow()->WorkRect.GetWidth();
		Result res = {};
		const float default_cursor_x = ImGui::GetCursorPos().x;
		bool declare_controls = ImGui::TreeNode("Parameters");
		if (declare_controls)
		{
			res |= declareControlsInline(ctx, available_width, ImGui::GetCursorPos().x - default_cursor_x);
			ImGui::TreePop();
			ImGui::Separator();
		}
		else
		{
			int array_layer = static_cast<int>(_array_layer);
			Range32i layers_range{ .begin = 0, .len = 1 };
			if (_latest_source_instance)
			{
				CalcLayersRange(_latest_source_instance.getRawVariant(), &layers_range, nullptr);
			}
			if (layers_range.len > 1)
			{
				if (ImGui::SliderInt("Layer", &array_layer, layers_range.begin, layers_range.end() - 1, nullptr, ImGuiSliderFlags_None))
				{
					_array_layer = array_layer;
					_manual_array_layer = true;
					clear();
				}
			}
		}
		{
			ImRect rect(ToIMGui(_uv_tl), ToIMGui(_uv_br));
			Vector2f old_size = _size_pix;
			calcDisplaySize(available_width);
			Vector2f new_size = _size_pix;
			if (new_size != old_size)
			{
				res |= Result::Resized;
			}
			bool could_show_image = declareImage(ctx, ToIMGui(_size_pix), &rect);
			if (could_show_image)
			{
				res |= Result::DisplayedImage;
				// TODO mouse zoom clip rect

				if (ImGui::BeginPopupContextItem("##RCM", ImGuiPopupFlags_MouseButtonRight))
				{
					res |= declareMenuInline(ctx);
					ImGui::EndPopup();
				}
			}
			else
			{
				ImVec4 col = ctx.style().invalid_red;
				ImGui::TextColored(col, "Cannot visualize image:");
				const char* reason = _error_message.empty() ? "Unknown reason." : _error_message.c_str();
				ImGui::TextColored(col, reason);
			}
		}
		return res;
	}

	ImageVisualizer::Result ImageVisualizer::declareMenuInline(Context& ctx)
	{
		Result res = {};
		bool should_clear = false;
		const bool use_submenus = false; // Can't decide which one looks better
		auto begin_named_section = [&](const char* label) -> bool
		{
			if (use_submenus)
			{
				return ImGui::BeginMenu(label);
			}
			else
			{
				ImGui::SeparatorText(label);
				return true;
			}
		};
		auto end_named_section = [&]()
		{
			if (use_submenus)
			{
				ImGui::EndMenu();
			}
		};
		auto declare_enum_option = [&]<class E>(E & current_value, const char* label, E option_value, const char* p_tooltip = nullptr, const char* shortcut = nullptr, bool disable = false)
		{
			bool selected = current_value == option_value;
			bool res = ImGui::MenuItem(label, shortcut, &selected, !disable);
			if (p_tooltip)
			{
				ImGui::SetItemTooltip(p_tooltip);
			}
			if (res)
			{
				current_value = option_value;
			}
			return res;
		};

		if (ImGui::MenuItem("Reset"))
		{
			_manual_format = false;
			_manual_swizzle = false;
			_manual_aspect = false;
			_manual_mips_range = false;
		}
		if (ImGui::MenuItem("Reset Crop"))
		{
			_uv_tl = Vector2f(0, 0);
			_uv_br = Vector2f(1, 1);
		}

		if(begin_named_section("Sizing"))
		{
			auto declare_size_mode = [&](const char* label, SizeMode size_mode, const char* p_tooltip = nullptr, const char* shortcut = nullptr, bool disable = false)
			{
				if (declare_enum_option(_size_mode, label, size_mode, p_tooltip, shortcut, disable))
				{
					res |= Result::Resized;
				}
			};
			declare_size_mode("Fit width (integral scaling)", SizeMode::FitWidthIntegral);
			declare_size_mode("Fit full width", SizeMode::FitWidth);
			declare_size_mode("Image resolution", SizeMode::ImageSize);
			declare_size_mode("Manual resolution", SizeMode::Manual);

			ImGui::Separator();
			{
				bool keep_ratio = !_manual_unlock_ratio;
				if (ImGui::MenuItem("Keep Ratio", nullptr, &keep_ratio))
				{
					_manual_unlock_ratio = !_manual_unlock_ratio;
					res |= Result::Resized;
				}
			}
			end_named_section();
		}
		if(begin_named_section("Sampler"))
		{
			auto declare_sampler_option = [&](const char* label, ImGuiSampler sampler_option, const char* p_tooltip = nullptr, const char* shortcut = nullptr, bool disable = false)
			{
				declare_enum_option(_imgui_sampler, label, sampler_option, p_tooltip, shortcut, disable);
			};
			declare_sampler_option("Current", ImGuiSampler::Default, "Use currently bound sampler.");
			declare_sampler_option("Nearest", ImGuiSampler::Nearest, "Use ImGui's Nearest sampler.");
			declare_sampler_option("Linear", ImGuiSampler::Linear, "Use ImGui's Linear sampler.");

			end_named_section();
		}

		if (should_clear)
		{
			clear();
		}
		return res;
	}

	ImageVisualizer::Result ImageVisualizer::declareControlsInline(Context& ctx, float available_width, float cursor_x_offset)
	{
		Result res = {};
		bool should_clear = false;
		if (ImGui::Button("Reset"))
		{
			_manual_format = false;
			_manual_swizzle = false;
			_manual_aspect = false;
			_manual_mips_range = false;
			should_clear |= true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Crop"))
		{
			_uv_tl = Vector2f(0, 0);
			_uv_br = Vector2f(1, 1);
		}
		ImGui::SameLine();
		if (ImGui::Button("Wide"))
		{
			if (_size_mode == SizeMode::FitWidthIntegral)
			{
				_size_mode = SizeMode::FitWidth;
			}
			else if (_size_mode == SizeMode::FitWidth)
			{
				_size_mode = SizeMode::ImageSize;
			}
			else
			{
				_size_mode = SizeMode::FitWidthIntegral;
			}
		}
		{
			ImGui::SameLine();
			bool locked = !_manual_unlock_ratio;
			if (ImGui::InboxCheckbox("Lock Ratio", &locked))
			{
				_manual_unlock_ratio = !locked;
				res |= Result::Resized;
			}
			Vector2i size_pix = _size_pix.cast<int>();
			bool input_size = false;
			if (available_width > 0 && !_manual_unlock_ratio)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() - cursor_x_offset);
				float item_width = ImMin(available_width, ImGui::GetContentRegionAvail().x);
				ImGui::SetNextItemWidth(item_width);
				input_size = ImGui::SliderInt("##Display width", size_pix.data(), 0, available_width, nullptr);
				ImGui::SetItemTooltip("Display width");
			}
			else
			{
				input_size = ImGui::DragInt2("Display Size", size_pix.data());
			}
			if (input_size)
			{
				_size_pix = size_pix.cast<float>();
				_size_mode = SizeMode::Manual;
				res |= Result::Resized;
			}
		}
		if (InspectSwizzleMapping(ctx, &_custom_swizzle))
		{
			_manual_swizzle = true;
			should_clear |= true;
		}


		ImGui::BeginDisabled(!_source);
		int array_layer = static_cast<int>(_array_layer);
		Range32i layers_range{.begin = 0, .len = 1};
		Range32i allowed_mips_range{.begin = 0, .len = 1};
		if (_latest_source_instance)
		{
			CalcLayersRange(_latest_source_instance.getRawVariant(), &layers_range, &allowed_mips_range);
		}
		if (ImGui::SliderInt("Layer", &array_layer, layers_range.begin, layers_range.end() - 1, nullptr, ImGuiSliderFlags_None))
		{
			array_layer = std::clamp(array_layer, layers_range.begin, layers_range.end() - 1);
			if (array_layer != _array_layer)
			{
				_array_layer = array_layer;
				should_clear |= true;
			}
			_manual_array_layer = true;
		}
		Range32i mips_range = _custom_mips_range.staticCastTo<i32>();
		if (InspectRange(ctx, "Mips", &mips_range, allowed_mips_range, true))
		{
			_custom_mips_range = mips_range.staticCastTo<u32>();
			_manual_mips_range = true;
			// TODO check if really necessary
			should_clear |= true;
		}
		ImGui::Separator();
		ImGui::EndDisabled();
		ImGui::ColorEdit4("Background", _background.data(), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
		ImGui::SameLine();
		ImGui::ColorEdit4("Tint", _tint.data(), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
		ImGui::SliderFloat2("Top-Left UV", _uv_tl.data(), 0, 1, nullptr, ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat2("Bottom-Right UV", _uv_br.data(), 0, 1, nullptr, ImGuiSliderFlags_NoRoundToFormat);
		if (should_clear)
		{
			clear();
		}

		if (_sampler)
		{
			std::shared_ptr<Sampler> new_sampler = {};
			bool change_sampler = _sampler_panel.declareInlineCheckType(ctx, _sampler, &new_sampler);
			if (change_sampler)
			{
				_sampler = std::move(new_sampler);
			}
		}
		else
		{
			uint index = static_cast<uint>(_imgui_sampler);
			std::array options = {
				ImGuiListSelection::OptionRaw{
					.label = "Current",
					.desc = "Use currently bound sampler.",
				},
				ImGuiListSelection::OptionRaw{
					.label = "Nearest",
					.desc = "Use ImGui's Nearest sampler.",
				},
				ImGuiListSelection::OptionRaw{
					.label = "Linear",
					.desc = "Use ImGui's Linear sampler.",
				},
				ImGuiListSelection::OptionRaw{
					.label = "Common",
					.desc = "Use shared sampler.",
				},
				ImGuiListSelection::OptionRaw{
					.label = "New",
					.desc = "Use new sampler.",
				},
			};
			ImGuiListSelection::DeclareInfoRaw sampler_list{
				.label = "Sampler##List",
				.options = options,
				.index = index,
			};
			int new_index = ImGuiListSelection::Declare(sampler_list, ImGuiListSelection::Mode::Dropdown);
			if (new_index >= 0)
			{
				const uint m = static_cast<uint>(ImGuiSampler::Count);
				if (static_cast<uint>(new_index) < m)
				{
					_imgui_sampler = static_cast<ImGuiSampler>(new_index);
				}
				else
				{
					const uint n = static_cast<uint>(new_index) - m;
					if (n == 0)
					{
						_sampler = ctx.sampler();
					}
					else if (n == 1)
					{
						_sampler = std::make_shared<Sampler>(Sampler::CI{
							.app = ctx.sampler()->application(),
							.name = _label + ".Sampler",
						});
					}
				}
			}
		}
		return res;
	}
}