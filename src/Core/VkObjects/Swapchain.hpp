#pragma once

#include <Core/App/VkApplication.hpp>

#include "Surface.hpp"
#include "ImageView.hpp"
#include "AbstractInstance.hpp"

namespace vkl
{
	class SwapchainInstance : public AbstractInstance
	{
	protected:
		
		static std::atomic<size_t> _total_instance_counter;
		static std::atomic<size_t> _alive_instance_counter;

		VkSwapchainCreateInfoKHR _ci = {};

		std::shared_ptr<Surface> _surface = nullptr;

		std::vector<std::shared_ptr<Image>> _images;
		std::vector<std::shared_ptr<ImageView>> _views;
		
		VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
		size_t _unique_id = 0;

		void create();

		void destroy();

		void setVkName();

	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Surface> surface = nullptr;
			VkSwapchainCreateInfoKHR ci;
		};
		using CI = CreateInfo;

		SwapchainInstance(CreateInfo const& ci);

		virtual ~SwapchainInstance() override;

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
			return swapchain();
		}

		constexpr const VkSwapchainCreateInfoKHR& createInfo()const
		{
			return _ci;
		}

		constexpr const std::vector<std::shared_ptr<Image>>&images()const
		{
			return _images;
		}

		constexpr const std::vector<std::shared_ptr<ImageView>>& views()const
		{
			return _views;
		}

	};

	class Swapchain : public InstanceHolder<SwapchainInstance>
	{
	public:
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Surface> surface = nullptr;
			uint32_t min_image_count = 0;
			DynamicValue<VkSurfaceFormatKHR> target_format;
			DynamicValue<VkExtent2D> extent;
			uint32_t layers = 1;
			VkImageUsageFlags image_usages = 0;
			std::vector<uint32_t> queues = {};
			VkSurfaceTransformFlagBitsKHR pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			DynamicValue<VkPresentModeKHR> target_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			bool clipped = true;
			std::shared_ptr<Swapchain> old_swapchain = nullptr;
		};
		using CI = CreateInfo;

	protected:


		DynamicValue<VkExtent2D> _extent;
		std::shared_ptr<Surface> _surface;
		std::vector<uint32_t> _queues = {};
		DynamicValue<VkPresentModeKHR> _target_present_mode;
		DynamicValue<VkSurfaceFormatKHR> _target_format;
		std::shared_ptr<SwapchainInstance> _old_swapchain = nullptr;

		VkSwapchainCreateInfoKHR _ci = {};

		void createInstance();

		void destroyInstance();

	public:

		Swapchain(CreateInfo const& ci);

		Swapchain(Swapchain const&) = delete;

		Swapchain& operator=(Swapchain const&) = delete;

		Swapchain(Swapchain&& other);

		Swapchain& operator=(Swapchain&&);
		
		virtual ~Swapchain() override;

		bool updateResources(UpdateContext & ctx);

		constexpr VkExtent2D getPossibleExtent(VkExtent2D target, VkSurfaceCapabilitiesKHR capabilities) const
		{
			{
				VkExtent2D res{
					.width = std::clamp(target.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
					.height = std::clamp(target.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
				};
				return res;
			}
		}

		constexpr const std::shared_ptr<SwapchainInstance>& instance()const
		{
			return _inst;
		}

		const DynamicValue<VkSurfaceFormatKHR> & format()const
		{
			return _target_format;
		}

		const DynamicValue<VkExtent2D> & extent() const
		{
			return _extent;
		}
		
	};
}