
#include "VkApplication.hpp"
#include <Core/VkObjects/CommandBuffer.hpp>

#include <exception>
#include <set>
#include <limits>

namespace vkl
{
	VkResult VkApplication::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}
	
	void VkApplication::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	std::vector<const char*> VkApplication::getValidLayers()
	{
		return {
			"VK_LAYER_KHRONOS_validation"
		};
	}

	std::vector<const char* > VkApplication::getDeviceExtensions()
	{
		return {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
			VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,
			VK_EXT_MESH_SHADER_EXTENSION_NAME,
		};
	}

	void VkApplication::requestFeatures(VulkanFeatures& features)
	{
		const VkBool32 t = VK_TRUE;
		const VkBool32 f = VK_FALSE;
		features.features_13.synchronization2 = t;
		features.features.geometryShader = t;
		features.features.samplerAnisotropy = t;

		features.features_12.shaderInt8 = t;
	
		features.features_12.descriptorBindingUniformBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingSampledImageUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageImageUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingUniformTexelBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageTexelBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingUpdateUnusedWhilePending = t;

		features.line_raster_ext.bresenhamLines = t;
		features.line_raster_ext.stippledBresenhamLines = t;

		features.index_uint8_ext.indexTypeUint8 = t;

		features.mesh_shader_ext.meshShader = t;
		features.mesh_shader_ext.taskShader = t;
		features.mesh_shader_ext.multiviewMeshShader = t;
		features.mesh_shader_ext.primitiveFragmentShadingRateMeshShader = t;
	}

	std::vector<const char*> VkApplication::getInstanceExtensions()
	{
		uint32_t glfw_ext_count = 0;
		const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
		std::vector<const char*> extensions(glfw_exts, glfw_exts + glfw_ext_count);
		if (_enable_valid_layers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		
		return extensions;
	}


	bool VkApplication::QueueFamilyIndices::isComplete()const
	{
		return graphics_family.has_value() && transfer_family.has_value() && present_family.has_value() && compute_family.has_value();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
	{
		if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			VK_LOG << "[VL]: " << callback_data->pMessage << std::endl << std::endl;
		return VK_FALSE;
	}

	bool VkApplication::checkValidLayerSupport()
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		const auto valid_layers = getValidLayers();

		bool res = std::all_of(valid_layers.cbegin(), valid_layers.cend(), [&available_layers](const char* layer_name)
			{
				return available_layers.cend() != std::find_if(available_layers.cbegin(), available_layers.cend(), [&layer_name](VkLayerProperties const& layer_prop)
					{
						return strcmp(layer_name, layer_prop.layerName) == 0;
					});
			});

		return res;
	}

	void VkApplication::preChecks()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		VK_LOG << extensionCount << " extensions supported\n";

		std::vector<VkExtensionProperties> available_extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, available_extensions.data());

		for (const auto& e : available_extensions)
		{
			VK_LOG << '\t' << e.extensionName << '\n';
		}


		if (_enable_valid_layers)
		{
			if (!checkValidLayerSupport())
			{
				throw std::runtime_error("Missing Validation Layer!");
			}
			VK_LOG << "Found All Required Validation Layers!\n";
		}
	}

	void VkApplication::initInstance(std::string const& name)
	{
		VkApplicationInfo app_info{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = name.c_str(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "None",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3,
		};

		auto extensions = getInstanceExtensions();
		VkInstanceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data(),
		};

		VkDebugUtilsMessengerCreateInfoEXT instance_debug_create_info{};

		const auto valid_layers = getValidLayers();

		if (_enable_valid_layers)
		{
			create_info.enabledLayerCount = valid_layers.size();
			create_info.ppEnabledLayerNames = valid_layers.data();

			populateDebugMessengerCreateInfo(instance_debug_create_info);
			create_info.pNext = &instance_debug_create_info;
		}
		else
		{
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &_instance), "Failed to create the Vulkan Instance");

		_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT");

	}

	void VkApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)const
	{
		create_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
		};
	}

	void VkApplication::initValidLayers()
	{
		if (_enable_valid_layers)
		{
			VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
			populateDebugMessengerCreateInfo(debug_create_info);

			VK_CHECK(CreateDebugUtilsMessengerEXT(_instance, &debug_create_info, nullptr, &_debug_messenger), "Failed to set up the debug messenger!");
		}
	}

	VkApplication::QueueFamilyIndices VkApplication::findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

		//VK_LOG << "Found " << queue_family_count << " queue(s).\n";

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		uint32_t i = 0;
		for (const auto& queue_family : queue_families)
		{
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphics_family = i;
			}
			if ((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) || (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)))
			{
				indices.transfer_family = i;
			}
			if ((queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				indices.compute_family = i;
			}
			if (vkGetPhysicalDeviceWin32PresentationSupportKHR(device, i) && !indices.present_family.has_value())
			{
				indices.present_family = i;
			}

			if (indices.isComplete())
				break;

			++i;
		}
		if (!indices.isComplete())
		{
			VK_LOG << "Warning, failed to find all the queue\n";
		}

		return indices;
	}

	uint32_t VkApplication::checkDeviceExtensionSupport(VkPhysicalDevice const& device)
	{
		const auto device_extensions = getDeviceExtensions();

		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		_device_extensions.resize(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, _device_extensions.data());

		for (auto it = _device_extensions.begin(); it != _device_extensions.end(); ++it)
		{
			if(!std::contains(device_extensions, std::string_view(it->extensionName)))
			{
				it = _device_extensions.erase(it);
				--it;
			}
		}

		assert(_device_extensions.size() <= device_extensions.size());
		
		return _device_extensions.size();
	}

	uint32_t VkApplication::getDeviceExtVersion(std::string_view ext_name) const
	{
		for (const auto& ext : _device_extensions)
		{
			if (ext.extensionName == ext_name)
			{
				return ext.specVersion;
			}
		}
		return EXT_NONE;
	}

	bool VkApplication::isDeviceSuitable(VkPhysicalDevice const& device)
	{
		bool res = true;

		VkBool32 present_support = false;
		QueueFamilyIndices indices = findQueueFamilies(device);

		//res = indices.isComplete();

		if (res)
		{
			//vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.present_family.value(), _surface, &present_support);
			res = vkGetPhysicalDeviceWin32PresentationSupportKHR(device, indices.present_family.value());
		}

		if (res)
		{
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);
			//res = features.samplerAnisotropy;
		}

		return res;
	}

	int64_t VkApplication::ratePhysicalDevice(VkPhysicalDevice const& device)
	{
		int64_t res = 0;

		VkPhysicalDeviceProperties2 props{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = nullptr, // TODO with new vulkan versions
		};
		VulkanFeatures features;
		VkPhysicalDeviceFeatures2 features2 = features.link();
		vkGetPhysicalDeviceProperties2(device, &props);
		vkGetPhysicalDeviceFeatures2(device, &features2);

		bool suitable = isDeviceSuitable(device);
		if (!suitable)
		{
			return std::numeric_limits<int64_t>::min();
		}

		int64_t discrete_multiplicator = (props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 8 : 1;

		// TODO count the available requested features

		res = 1;

		res *= discrete_multiplicator;

		VK_LOG << "Rated " << props.properties.deviceName << ": " << res << "\n";

		return res;
	}

	void VkApplication::pickPhysicalDevice()
	{
		uint32_t physical_device_count = 0;
		vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr);

		if (physical_device_count == 0)
		{
			throw std::runtime_error("No physical device found!");
		}

		std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
		vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.data());

		VK_LOG << "Found " << physical_device_count << " physical device(s)\n";

		auto it = std::findBest(physical_devices.cbegin(), physical_devices.cend(), [&](VkPhysicalDevice const& d) {return ratePhysicalDevice(d); });
		_physical_device = *it;
		size_t device_index = it - physical_devices.cbegin();
		//_physical_device = physical_devices[1];

		VkPhysicalDeviceProperties2 _physical_device_propeties = _device_props.link();
		vkGetPhysicalDeviceProperties2(_physical_device, &_physical_device_propeties);
		_device_props.props = _physical_device_propeties.properties;
		checkDeviceExtensionSupport(_physical_device);
		_queue_family_indices = findQueueFamilies(_physical_device);

		VK_LOG << "Using " << _physical_device_propeties.properties.deviceName << " as physical device.\n";
	}

	void VkApplication::createLogicalDevice()
	{
		QueueFamilyIndices& indices = _queue_family_indices;

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families;
		const auto addQueueIFP = [&](std::optional<uint32_t> const& i)
		{
			if (i.has_value())	unique_queue_families.insert(unique_queue_families.end(), i.value());
		};
		addQueueIFP(indices.graphics_family);
		addQueueIFP(indices.transfer_family);
		addQueueIFP(indices.present_family);
		addQueueIFP(indices.compute_family);

		float priority = 1;
		for (uint32_t unique_queue_family : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = unique_queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VulkanFeatures exposed_device_features;
		VkPhysicalDeviceFeatures2 physical_device_features2 = exposed_device_features.link();
		vkGetPhysicalDeviceFeatures2(_physical_device, &physical_device_features2);
		exposed_device_features.features = physical_device_features2.features;

		_available_features = filterFeatures(_requested_features, exposed_device_features);

		VkPhysicalDeviceFeatures2 features2 = _available_features.link();

		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext = &features2;
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
		device_create_info.queueCreateInfoCount = queue_create_infos.size();
		device_create_info.pEnabledFeatures = nullptr;

		device_create_info.enabledExtensionCount = _device_extensions.size();
		std::vector<const char*> device_extensions(_device_extensions.size());
		for (size_t e = 0; e < device_extensions.size(); ++e)
		{
			device_extensions[e] = _device_extensions[e].extensionName;
		}
		device_create_info.ppEnabledExtensionNames = device_extensions.data();

		const auto valid_layers = getValidLayers();
		if (_enable_valid_layers)
		{
			device_create_info.enabledLayerCount = valid_layers.size();
			device_create_info.ppEnabledLayerNames = valid_layers.data();
		}
		else
		{
			device_create_info.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(_physical_device, &device_create_info, nullptr, &_device), "Failed to create the logical device!");

		if (indices.graphics_family.has_value()) vkGetDeviceQueue(_device, indices.graphics_family.value(), 0, &_queues.graphics);
		if (indices.present_family.has_value()) vkGetDeviceQueue(_device, indices.present_family.value(), 0, &_queues.present);
		if (indices.transfer_family.has_value()) vkGetDeviceQueue(_device, indices.transfer_family.value(), 0, &_queues.transfer);
		if (indices.compute_family.has_value()) vkGetDeviceQueue(_device, indices.compute_family.value(), 0, &_queues.compute);


		loadExtFunctionsPtr();
	}

	void VkApplication::loadExtFunctionsPtr()
	{
		if (_available_features.mesh_shader_ext.meshShader)
		{
			_ext_functions._vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT");
		}
	}


	void VkApplication::createCommandPools()
	{
		const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		if (_queue_family_indices.graphics_family.has_value()) _pools.graphics = std::make_shared<CommandPool>(this, _queue_family_indices.graphics_family.value(), flags);
		if (_queue_family_indices.transfer_family.has_value()) _pools.transfer = std::make_shared<CommandPool>(this, _queue_family_indices.transfer_family.value(), flags);
		if (_queue_family_indices.compute_family.has_value()) _pools.compute = std::make_shared<CommandPool>(this, _queue_family_indices.compute_family.value(), flags);
	}

	void VkApplication::createAllocator()
	{
		VmaAllocatorCreateInfo alloc_ci{
			.physicalDevice = _physical_device,
			.device = _device,
			.instance = _instance,
			.vulkanApiVersion = VK_API_VERSION_1_2,
		};
		vmaCreateAllocator(&alloc_ci, &_allocator);
		//_staging_pool = std::make_unique<StagingPool>(this, _allocator);
	}

	void VkApplication::nameObject(VkDebugUtilsObjectNameInfoEXT const& object_to_name)
	{
		//std::cout << "nameObject: not yet implemented (" << std::hex << object_to_name.object << std::dec << " -> " << object_to_name.pObjectName << ")\n";
		if (_enable_valid_layers)
		{
			VK_CHECK(_vkSetDebugUtilsObjectNameEXT(_device, &object_to_name), "Failed to name an object.");
		}
	}

	void VkApplication::initGLFW()
	{
		glfwInit();
	}

	VkApplication::VkApplication(std::string const& name, bool validation) :
		_enable_valid_layers(validation),
		_name(name)
	{

	}

	void VkApplication::init()
	{
		initGLFW();
		preChecks();

		requestFeatures(_requested_features);

		initInstance(_name);
		initValidLayers();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createCommandPools();
	}

	void VkApplication::cleanup()
	{
		_pools.graphics = nullptr;
		_pools.transfer = nullptr;
		_pools.compute = nullptr;

		//_staging_pool = nullptr;
		vmaDestroyAllocator(_allocator);

		vkDestroyDevice(_device, nullptr);

		if (_enable_valid_layers)
		{
			DestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
		}

		vkDestroyInstance(_instance, nullptr);

		glfwTerminate();
	}

	VkApplication::QueueFamilyIndices const& VkApplication::getQueueFamilyIndices()const
	{
		return _queue_family_indices;
	}

	VkApplication::Queues const& VkApplication::queues()const
	{
		return _queues;
	}

	VkApplication::Pools const& VkApplication::pools()const
	{
		return _pools;
	}

	//StagingPool& VkApplication::stagingPool()
	//{
	//	return *_staging_pool;
	//}

	VkApplication::~VkApplication()
	{
		cleanup();
	}
}
