#include "VkApplication.hpp"

#include "Surface.hpp"
#include "ImageView.hpp"

namespace vkl
{
	class Swapchain : public VkObject
	{
	public:
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Surface> surface = nullptr;
			uint32_t min_image_count = 0;
			VkFormat target_format = VK_FORMAT_MAX_ENUM;
			VkColorSpaceKHR target_color_space = VK_COLOR_SPACE_MAX_ENUM_KHR;
			VkExtent2D extent = makeZeroExtent2D();
			uint32_t layers = 1;
			VkImageUsageFlags image_usages = 0;
			std::vector<uint32_t> queues = {};
			VkSurfaceTransformFlagsKHR pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			VkPresentModeKHR target_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			bool clipped = true;
			std::shared_ptr<Swapchain> old_swapchain = nullptr;
		};
		using CI = CreateInfo;

	protected:

		std::shared_ptr<Surface> _surface;
		std::vector<uint32_t> _queues = {};
		std::vector<std::shared_ptr<Image>> _images;
		std::vector<std::shared_ptr<ImageView>> _views;
		
		VkSwapchainCreateInfoKHR _ci = {};


		VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

		void create();

		void destroy();

	public:

		constexpr Swapchain() = default;


		Swapchain(CreateInfo const& ci);

		Swapchain(Swapchain const&) = delete;

		Swapchain& operator=(Swapchain const&) = delete;

		Swapchain(Swapchain&& other);

		Swapchain& operator=(Swapchain&&);
		

		virtual ~Swapchain();


		constexpr VkSwapchainKHR swapchain()const
		{
			return _swapchain;
		}

		constexpr VkSwapchainKHR handle()const
		{
			return swapchain();
		}

		constexpr operator VkSwapchainKHR()const
		{
			swapchain();
		}

		constexpr VkFormat format()const
		{
			return _ci.imageFormat;
		}

		constexpr const std::vector<std::shared_ptr<Image>> & images()const
		{
			return _images;
		}

		constexpr const std::vector<std::shared_ptr<ImageView>>& views()const
		{
			return _views;
		}
		
	};
}