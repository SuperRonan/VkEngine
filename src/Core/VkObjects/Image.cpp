#include "Image.hpp"

namespace vkl
{
	std::atomic<size_t> ImageInstance::_instance_counter = 0;

	void ImageInstance::setVkNameIFP()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT object_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _name.data(),
			};
			_app->nameObject(object_name);
		}
	}

	void ImageInstance::create()
	{
		assert(_image == VK_NULL_HANDLE);

		VK_CHECK(vmaCreateImage(_app->allocator(), &_ci, &_vma_ci, &_image, &_alloc, nullptr), "Failed to create an image.");

		setVkNameIFP();
	}

	void ImageInstance::destroy()
	{
		assert(_image != VK_NULL_HANDLE);

		callDestructionCallbacks();

		if (ownership())
		{
			vmaDestroyImage(_app->allocator(), _image, _alloc);
		}

		_image = VK_NULL_HANDLE;
		_alloc = nullptr;
	}

	ImageInstance::ImageInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_ci(ci.ci),
		_vma_ci(ci.aci),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{
		create();
	}

	ImageInstance::ImageInstance(AssociateInfo const& ai) :
		AbstractInstance(ai.app, ai.name),
		_ci(ai.ci),
		_image(ai.image),
		_unique_id(std::atomic_fetch_add(&_instance_counter, 1))
	{

	}

	ImageInstance::~ImageInstance()
	{
		if (!!_image)
		{
			destroy();
		}
	}


	Image::Image(CreateInfo const& ci) : 
		InstanceHolder<ImageInstance>(ci.app, ci.name),
		_flags(ci.flags),
		_type(ci.type),
		_format(ci.format),
		_extent(ci.extent),
		_mips(ci.mips == ALL_MIPS ? Image::howManyMips(ci.type, *ci.extent) : ci.mips),
		_layers(ci.layers),
		_samples(ci.samples),
		_tiling(ci.tiling),
		_usage(ci.usage),
		_sharing_mode(ci.queues.size() <= 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
		_queues(ci.queues),
		_initial_layout(ci.initial_layout),
		_mem_usage(ci.mem_usage)
	{
		if(ci.create_on_construct)
			createInstance();
	}

	Image::Image(AssociateInfo const& assos):
		InstanceHolder<ImageInstance>(assos.instance->application(), assos.instance->name())
	{
		associateImage(assos);
	}

	void Image::createInstance()
	{
		if (_inst)
		{
			destroyInstance();
		}
		uint32_t n_queues = 0;
		uint32_t* p_queues = nullptr;
		if (_sharing_mode == VK_SHARING_MODE_CONCURRENT)
		{	
			n_queues = _queues.size();
			p_queues = _queues.data();
		}

		VkImageCreateInfo image_ci{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = _flags,
			.imageType = _type,
			.format = _format,
			.extent = *_extent,
			.mipLevels = _mips,
			.arrayLayers = _layers,
			.samples = _samples,
			.tiling = _tiling,
			.usage = _usage,
			.sharingMode = _sharing_mode,
			.queueFamilyIndexCount = n_queues,
			.pQueueFamilyIndices = p_queues,
			.initialLayout = _initial_layout,
		};

		VmaAllocationCreateInfo alloc{
			.usage = _mem_usage,
		};

		_inst = std::make_shared<ImageInstance>(ImageInstance::CI
		{
			.app = _app,
			.name = name(),
			.ci = image_ci,
			.aci = alloc,
		});
	}

	void Image::associateImage(AssociateInfo const& assos)
	{
		assert(_inst == nullptr);
		_app = assos.instance->application();
		_inst = assos.instance;
		if (!assos.instance->name().empty())
			_name = assos.instance->name().empty();

		_type = assos.instance->createInfo().imageType;
		_format = assos.instance->createInfo().format;
		_extent = assos.extent;
		_mips = assos.instance->createInfo().mipLevels;
		_layers = assos.instance->createInfo().arrayLayers; // TODO Check if the swapchain can support multi layered images (VR)
		_samples = assos.instance->createInfo().samples;
		_usage = assos.instance->createInfo().usage;
		_queues = std::vector<uint32_t>(assos.instance->createInfo().pQueueFamilyIndices, assos.instance->createInfo().pQueueFamilyIndices + assos.instance->createInfo().queueFamilyIndexCount);
		_sharing_mode = (_queues.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		_mem_usage = assos.instance->AllocationInfo().usage;
		_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED; // In which layout are created swapchain images?+
	}

	void Image::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	//StagingPool::StagingBuffer* Image::copyToStaging2D(StagingPool& pool, void* data, uint32_t elem_size)
	//{
	//	assert(_type == VK_IMAGE_TYPE_2D);
	//	size_t size = _extent.width * _extent.height * elem_size;
	//	StagingPool::StagingBuffer* sb = pool.getStagingBuffer(size);
	//	std::memcpy(sb->data, data, size);
	//	return sb;
	//}


	bool Image::updateResource(UpdateContext & ctx)
	{
		using namespace vk_operators;
		bool res = false;
		if (_inst)
		{
			if (_inst->ownership())
			{
				const VkExtent3D new_extent = *_extent;

				if (new_extent != _inst->createInfo().extent)
				{
					destroyInstance();
					res = true;
				}
			}
		}

		if (!_inst)
		{
			createInstance();
			res = true;
		}


		return res;
	}
}