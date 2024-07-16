#pragma once

#include <vkl/App/VkApplication.hpp>

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

		uint64_t _present_id_counter = 0;

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

		VkSurfaceFormatKHR format()const
		{
			return VkSurfaceFormatKHR{
				.format = _ci.imageFormat,
				.colorSpace = _ci.imageColorSpace,
			};	
		}

		constexpr const std::vector<std::shared_ptr<Image>>&images()const
		{
			return _images;
		}

		constexpr const std::vector<std::shared_ptr<ImageView>>& views()const
		{
			return _views;
		}

		constexpr uint64_t presentIdCounter() const
		{
			return _present_id_counter;
		}

		constexpr uint64_t getNextPresentId()
		{
			uint64_t res = _present_id_counter;
			++_present_id_counter;
			return res;
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
			Dyn<VkSurfaceFormatKHR> target_format = {};
			Dyn<VkExtent2D> extent = {};
			uint32_t layers = 1;
			VkImageUsageFlags image_usages = 0;
			std::vector<uint32_t> queues = {};
			VkSurfaceTransformFlagBitsKHR pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			Dyn<VkPresentModeKHR> target_present_mode = {};
			bool clipped = true;
			std::shared_ptr<Swapchain> old_swapchain = nullptr;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:


		Dyn<VkExtent2D> _extent;
		std::shared_ptr<Surface> _surface;
		std::vector<uint32_t> _queues = {};
		Dyn<VkPresentModeKHR> _target_present_mode;
		Dyn<VkSurfaceFormatKHR> _target_format;
		std::shared_ptr<SwapchainInstance> _old_swapchain = nullptr;

		VkSwapchainCreateInfoKHR _ci = {};

		void createInstance();

		virtual void destroyInstanceIFN() override;

	public:

		Swapchain(CreateInfo const& ci);

		Swapchain(Swapchain const&) = delete;

		Swapchain& operator=(Swapchain const&) = delete;
		
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

		const Dyn<VkSurfaceFormatKHR> & format()const
		{
			return _target_format;
		}

		const Dyn<VkExtent2D> & extent() const
		{
			return _extent;
		}

		const Dyn<VkPresentModeKHR>& presentMode()const
		{
			return _target_present_mode;
		}
		
	};
}