#pragma once

#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Sampler.hpp>
#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/GUI/TypedInlineInspector.hpp>

class ImRect;

namespace vkl::GUI
{

	class ImageVisualizer
	{
	public:

		enum class ImGuiSampler : char
		{
			Default,
			Nearest,
			Linear,
			Count
		};

		enum class SizeMode : char
		{
			FitWidthIntegral, // Fit to the available window width, but keep integral proportions
			FitWidth, // Fit to the available window width
			ImageSize,
			Manual,
		};

		using SourcePtr = RawPointerVariant<Image, ImageInstance, ImageView, ImageViewInstance>;
		using SourceSPtr = SharedPointerVariant<Image, ImageInstance, ImageView, ImageViewInstance>;
		using InstancePtr = that::RawPointerDynamicVariant<AbstractInstance, ImageInstance, ImageViewInstance>;
		using InstanceSPtr = that::SharedPointerDynamicVariant<AbstractInstance, ImageInstance, ImageViewInstance>;

		static ImageInstance* GetImageInstance(InstancePtr ptr);

	protected:

		std::string _label;
		
		// May be one of: Image, ImageInstance, ImageView, ImageViewInstance
		SourceSPtr _source = {};
		InstanceSPtr _latest_source_instance = {};

		VkFormat _custom_format = VK_FORMAT_UNDEFINED;
		VkComponentMapping _custom_swizzle = defaultComponentMapping();
		VkImageAspectFlags _custom_aspect = 0;
		Range32u _custom_mips_range = {};
		uint32_t _array_layer = 0;

		bool _manual_format : 1 = false;
		bool _manual_swizzle : 1 = false;
		bool _manual_aspect : 1 = false;
		bool _manual_mips_range : 1 = false;
		bool _manual_array_layer : 1 = false;
		bool _manual_unlock_ratio : 1 = false;
		ImGuiSampler _imgui_sampler = ImGuiSampler::Default;
		SizeMode _size_mode = SizeMode::ImageSize;

		std::shared_ptr<ImageViewInstance> _custom_view = {};
		std::shared_ptr<ImageView> _custom_view_desc = {};

		// Since ImGui 1.92.8, the texture and sampler are stored in a different set
		// TODO use the same pool for the two sets
		std::shared_ptr<DescriptorSetAndPoolInstance> _texture_set;

		// Optional, If not used, the default imgui sampler is used
		std::shared_ptr<Sampler> _sampler;
		TypedInlineInspector<Sampler> _sampler_panel;
		std::shared_ptr<DescriptorSetAndPoolInstance> _sampler_set;

		Vector2f _size_pix; // Display size
		Vector2f _uv_tl = Vector2f(0, 0), _uv_br = Vector2f(1, 1);
		Vector4f _background = Vector4f(0, 0, 0, 0);
		Vector4f _tint = Vector4f(1, 1, 1, 1);

		std::string _error_message = {};

		void createTextureSet(std::shared_ptr<DescriptorSetLayoutInstance> const& layout);
		void createSamplerSet(std::shared_ptr< DescriptorSetLayoutInstance> const& layout);

		void createDefaultView();

		void createCurstomView(std::shared_ptr<ImageInstance> const& image);

		void clear(bool keep_error_message=false);

		void checkInstance(Context& ctx);

		void calcDisplaySize(Vector2f image_size, float available_width);

		void calcDisplaySize(float available_width);

	public:

		struct CreateInfo
		{
			Context* ctx;
			std::string_view label;
		};
		using CI = CreateInfo;
		ImageVisualizer(CreateInfo const& ci);

		virtual ~ImageVisualizer();

		ImageVisualizer(ImageVisualizer const&) = delete;

		static bool CheckSourceType(const VkObject* source)
		{
			bool res = false;
			if (source)
			{
				res = SourcePtr::CheckDynamicType(source);
			}
			else
			{
				res = true;
			}
			return res;
		}

		void setSource(SourceSPtr const& source);

		virtual bool declareImage(Context& ctx, ImVec2 const& size, const ImRect * rect = nullptr, bool skip_registration=false);

		enum class Result : uint
		{
			None = 0x0,
			DisplayedImage = 0x1,
			Resized = 0x2,
		};

		virtual Result declareInline(Context& ctx);

		virtual Result declareControlsInline(Context& ctx, float available_width = -1, float cursor_x_offset = 0);

		virtual Result declareMenuInline(Context& ctx);

		std::string_view label() const
		{
			return _label;
		}
	};

THAT_DECLARE_ENUM_CLASS_OPERATORS(ImageVisualizer::Result);
}