#include "Swapchain.hpp"
#include <cassert>
#include <utility>

namespace vkl
{
	void SwapchainInstance::create()
	{
		assert(_swapchain == VK_NULL_HANDLE);
		VK_CHECK(vkCreateSwapchainKHR(_app->device(), &_ci, nullptr, &_swapchain), "Failed to create a swapchain.");

		uint32_t image_count;
		vkGetSwapchainImagesKHR(_app->device(), _swapchain, &image_count, nullptr);
		_images.resize(image_count);
		_views.resize(image_count);
		std::vector<VkImage> tmp(image_count);
		vkGetSwapchainImagesKHR(_app->device(), _swapchain, &image_count, tmp.data());


		for (uint32_t i = 0; i < image_count; ++i)
		{
			ImageInstance::AssociateInfo instance_assos{
				.app = application(),
				.name = name() + std::string(".imageInstance_") + std::to_string(i),
				.ci = VkImageCreateInfo{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.imageType = VK_IMAGE_TYPE_2D, // Sure about that?
					.format = _ci.imageFormat,
					.extent = extend(_ci.imageExtent, 1),
					.mipLevels = 0,
					.arrayLayers = _ci.imageArrayLayers, // What if not 1?
					.samples = VK_SAMPLE_COUNT_1_BIT, // Sure about that?
					.tiling = VK_IMAGE_TILING_MAX_ENUM, // Don't know
					.usage = _ci.imageUsage,
					.sharingMode = _ci.imageSharingMode,
					.queueFamilyIndexCount = _ci.queueFamilyIndexCount,
					.pQueueFamilyIndices = _ci.pQueueFamilyIndices,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				},
				.image = tmp[i],
			};

			std::shared_ptr<ImageInstance> inst = std::make_shared<ImageInstance>(instance_assos);
			Image::AssociateInfo assos{
				.instance = inst,
				.extent = extend(_ci.imageExtent, 1),
			};

			_images[i] = std::make_shared<Image>();
			_images[i]->associateImage(assos);
			_views[i] = std::make_shared<ImageView>(ImageView::CI{
				.name = name() + ".view_" + std::to_string(i),
				.image = _images[i],
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			});
		}
	}

	void SwapchainInstance::destroy()
	{
		assert(_swapchain != VK_NULL_HANDLE);

		_views.clear();
		_images.clear();
		vkDestroySwapchainKHR(device(), _swapchain, nullptr);
		_swapchain = VK_NULL_HANDLE;
	}

	void SwapchainInstance::setName()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
				.objectHandle = (uint64_t)_swapchain,
				.pObjectName = name().data(),
			};
			_app->nameObject(object_name);
		}
	}

	SwapchainInstance::SwapchainInstance(CreateInfo const& ci):
		VkObject(ci.app, std::move(ci.name)),
		_ci(ci.ci),
		_surface(ci.surface)
	{
		create();
	}

	SwapchainInstance::~SwapchainInstance()
	{
		if (_swapchain)
		{
			destroy();
		}
	}

	bool Swapchain::reCreate()
	{
		std::shared_ptr<SwapchainInstance> old_swap = _inst;
		_ci.oldSwapchain = *old_swap;
		const Surface::SwapchainSupportDetails& support = _surface->getDetails();
		_surface->queryDetails();
		_ci.imageExtent = getPossibleExtent(_extent, support.capabilities);

		createInstance();
		return true;
	}

	void Swapchain::createInstance()
	{
		_inst = std::make_shared<SwapchainInstance>(SwapchainInstance::CI{
			.app = application(),
			.name = name(),
			.ci = _ci,
		});
	}

	void Swapchain::destroyInstance()
	{
		_inst = nullptr;
	}

	Swapchain::~Swapchain()
	{
		destroyInstance();
	}

	Swapchain::Swapchain(CreateInfo const& ci) :
		AbstractInstanceHolder(ci.app, ci.name),
		_extent(ci.extent),
		_queues(ci.queues),
		_surface(ci.surface)
	{
		const Surface::SwapchainSupportDetails & support = _surface->getDetails();

		const uint32_t n_queues = _queues.size();
		const uint32_t* p_queues = n_queues ? _queues.data() : nullptr;
		const VkSharingMode sharing_mode = (n_queues > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

		
		_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		_ci.pNext = nullptr;
		_ci.flags = 0;
		_ci.surface = *_surface;
		_ci.minImageCount = std::max(support.capabilities.minImageCount, ci.min_image_count);
		const VkSurfaceFormatKHR fmt = [&]()
		{
			VkSurfaceFormatKHR res = support.formats.front();
			uint32_t score = 0;
			for (size_t i = 0; i < support.formats.size(); ++i)
			{
				const VkSurfaceFormatKHR tmp = support.formats[i];
				uint32_t tmp_score = 0;
				if (tmp.format == ci.target_format) tmp_score += 1;
				if (tmp.colorSpace == ci.target_color_space) tmp_score += 1;
				if (tmp_score > score)
				{
					score = tmp_score;
					res = tmp;
				}
			}
			return res;
		}();
		_ci.imageFormat = fmt.format;
		_ci.imageColorSpace = fmt.colorSpace;
		_ci.imageExtent = getPossibleExtent(_extent, support.capabilities);
		_ci.imageArrayLayers = ci.layers;
		_ci.imageUsage = ci.image_usages;
		_ci.imageSharingMode = sharing_mode;
		_ci.queueFamilyIndexCount = n_queues;
		_ci.pQueueFamilyIndices = p_queues;
		_ci.preTransform = ci.pre_transform;
		_ci.compositeAlpha = ci.composite_alpha;
		_ci.presentMode = [&]() -> VkPresentModeKHR
		{
			if (std::find(support.present_modes.cbegin(), support.present_modes.cend(), ci.target_present_mode) != support.present_modes.cend())
			{
				return ci.target_present_mode;
			}
			else
			{
				return support.present_modes.front();
			}
		}();
		_ci.clipped = ci.clipped;
		_ci.oldSwapchain = VK_NULL_HANDLE;
	}

	bool Swapchain::updateResources()
	{
		if (!_inst)
		{
			createInstance();
		}
		bool res = false;
		for (auto& view : _inst->views())
		{
			res |= view->updateResource();
		}
		return res;
	}
}