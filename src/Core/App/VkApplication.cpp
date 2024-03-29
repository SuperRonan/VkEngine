
#include "VkApplication.hpp"
#include <Core/VkObjects/CommandBuffer.hpp>
#include <Core/Execution/SamplerLibrary.hpp>
#include <Core/Rendering/TextureFromFile.hpp>
#include <Core/VkObjects/DescriptorSetLayout.hpp>

#include <Core/Commands/PrebuiltTransferCommands.hpp>

#include <Core/IO/File.hpp>

#include <argparse/argparse.hpp>

#include <exception>
#include <set>
#include <limits>
#include <fstream>


namespace vkl
{

	void VkApplication::FillArgs(argparse::ArgumentParser& args)
	{
		args.add_argument("--name")
			.help("Name of the Application")
		;

		int default_validation = 0;
		int default_name_vk_objects = 0;
		int default_cmd_labels = 0;

#if _DEBUG
		default_validation = 1;
		default_name_vk_objects = 1;
		default_cmd_labels = 1;
#endif

		args.add_argument("--validation")
			.help("Enable Vulkan Validation Layers")
			.default_value(default_validation)
			.scan<'d', int>()
		;

		args.add_argument("--name_vk_objects")
			.help("Name Vulkan Objects")
			.default_value(default_name_vk_objects)
			.scan<'d', int>()
		;

		args.add_argument("--cmd_labels")
			.help("Set Labels in the CommandBuffer")
			.default_value(default_cmd_labels)
			.scan<'d', int>()
		;

		args.add_argument("--helper_threads")
			.help("Set the number of helper threads, 'all' to use all available threads, 'none' to run single thread, -n to use all threads minus n")
			.default_value("all"s)
		;

		args.add_argument("--gpu")
			.help("Select the index of the gpu to use")
			.default_value(-1)
			.scan<'d', int>()
		;
	}


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
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		};
	}

	void VkApplication::requestFeatures(VulkanFeatures& features)
	{
		const VkBool32 t = VK_TRUE;
		const VkBool32 f = VK_FALSE;
		features.features_13.synchronization2 = t;
		features.features2.features.geometryShader = t;
		
		features.features2.features.samplerAnisotropy = t;
		features.features_12.samplerMirrorClampToEdge = t;

		features.features2.features.multiDrawIndirect = t;
		features.features_11.shaderDrawParameters = t;


		features.features_12.shaderInt8 = t;
	
		features.features_12.descriptorBindingPartiallyBound = t;
		features.features_12.descriptorBindingUniformBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingSampledImageUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageImageUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingUniformTexelBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingStorageTexelBufferUpdateAfterBind = t;
		features.features_12.descriptorBindingUpdateUnusedWhilePending = t;

		features.features_12.runtimeDescriptorArray = t;

		features.features_13.maintenance4 = t;

		features.line_raster_ext.bresenhamLines = t;
		features.line_raster_ext.stippledBresenhamLines = t;

		features.index_uint8_ext.indexTypeUint8 = t;

		features.mesh_shader_ext.meshShader = t;
		features.mesh_shader_ext.taskShader = t;
		features.mesh_shader_ext.multiviewMeshShader = t;
		features.mesh_shader_ext.primitiveFragmentShadingRateMeshShader = t;

		features.robustness2_ext.nullDescriptor = t;

		features.features_13.dynamicRendering = t;
	}

	std::vector<const char*> VkApplication::getInstanceExtensions()
	{
		uint32_t sdl_ext_count = 0;
		SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_ext_count, nullptr);
		std::vector<const char*> extensions(sdl_ext_count, nullptr);
		SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_ext_count, extensions.data());
		if (_options.enable_validation || _options.enable_object_naming || _options.enable_command_buffer_labels)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
		
		return extensions;
	}


	bool VkApplication::QueueFamilyIndices::isComplete()const
	{
		return graphics_family.has_value() && transfer_family.has_value() && present_family.has_value() && compute_family.has_value();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
	{
		if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			VK_LOG << "[VL]: " << callback_data->pMessage << std::endl << std::endl;

 			int _ = 0;
		}
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


		if (_options.enable_validation)
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
		// TODO check that extensions are available
		VkInstanceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data(),
		};
		_instance_extensions.resize(extensions.size());
		for (size_t i = 0; i < extensions.size(); ++i)
		{
			// TODO automatically someday
			if (strcmp(extensions[i], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
			{
				_instance_extensions.push_back(VkExtensionProperties{
					.extensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					.specVersion = VK_EXT_DEBUG_UTILS_SPEC_VERSION,
				});
			}
		}

		VkDebugUtilsMessengerCreateInfoEXT instance_debug_create_info{};

		const auto valid_layers = getValidLayers();

		if (_options.enable_validation)
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
		if (_options.enable_validation)
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
		const auto requested_extensions = getDeviceExtensions();

		uint32_t extension_count;
		std::vector<VkExtensionProperties> queried_extensions;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		queried_extensions.resize(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, queried_extensions.data());

		_device_extensions.clear();

		uint32_t present = 0;
		uint32_t missing = 0;
		for (size_t i = 0; i < requested_extensions.size(); ++i)
		{
			size_t j = [&]() {
				for (size_t j = 0; j < queried_extensions.size(); ++j)
				{
					if (std::string_view(requested_extensions[i]) == std::string_view(queried_extensions[j].extensionName))
					{
						return j;
					}
				}
				return size_t(-1);
			}();
			if (j != -1)
			{
				++present;
				_device_extensions.push_back(queried_extensions[j]);
			}
			else
			{
				++missing;
			}
		}
		
		return present;
	}

	uint32_t VkApplication::getDeviceExtVersion(std::string_view ext_name) const
	{
		// TODO use a map
		for (const auto& ext : _device_extensions)
		{
			if (ext.extensionName == ext_name)
			{
				return ext.specVersion;
			}
		}
		return EXT_NONE;
	}

	uint32_t VkApplication::getInstanceExtVersion(std::string_view ext_name) const
	{
		// TODO use a map
		for (const auto& ext : _instance_extensions)
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

		VulkanDeviceProps props;
		VulkanFeatures features;
		vkGetPhysicalDeviceProperties2(device, &props.link());
		vkGetPhysicalDeviceFeatures2(device, &features.link());

		bool suitable = isDeviceSuitable(device);
		if (!suitable)
		{
			return std::numeric_limits<int64_t>::min();
		}

		uint32_t min_version = VK_MAKE_VERSION(1, 3, 0);

		int64_t discrete_multiplicator = (props.props2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 2 : 1;

		// TODO count the available requested features

		res = 1;

		if (props.props2.properties.apiVersion < min_version)
		{
			res = 0;
		}

		res *= discrete_multiplicator;

		VK_LOG << "Rated " << props.props2.properties.deviceName << ": " << res << "\n";

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

		VK_LOG << "Found " << physical_device_count << " physical device(s):\n";
		for (size_t i=0; i < physical_devices.size(); ++i)
		{
			VulkanDeviceProps props;
			vkGetPhysicalDeviceProperties2(physical_devices[i], &props.link());
			VK_LOG << " - " << i << ": " << props.props2.properties.deviceName << std::endl;
		}

		if (_options.gpu_id < physical_device_count)
		{
			_physical_device = physical_devices[_options.gpu_id];
		}
		else
		{
			auto it = std::findBest(physical_devices.cbegin(), physical_devices.cend(), [&](VkPhysicalDevice const& d) {return ratePhysicalDevice(d); });
			_physical_device = *it;

		}

		vkGetPhysicalDeviceProperties2(_physical_device, &_device_props.link());
		uint32_t supported_extensions = checkDeviceExtensionSupport(_physical_device);
		_queue_family_indices = findQueueFamilies(_physical_device);

		VK_LOG << "Using " << _device_props.props2.properties.deviceName << " as physical device.\n";
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
		vkGetPhysicalDeviceFeatures2(_physical_device, &exposed_device_features.link());

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
		if (_options.enable_validation)
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

		queryDescriptorBindingOptions();
	}

	void VkApplication::loadExtFunctionsPtr()
	{
		if (_available_features.mesh_shader_ext.meshShader)
		{
			_ext_functions._vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT"));
		}

		const bool has_debug_utils_ext = hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		if (_options.enable_object_naming)
		{
			if (has_debug_utils_ext)
			{
				_ext_functions._vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT"));
			}
			else
			{
				_options.enable_object_naming = false;
			}
		}

		if (_options.enable_command_buffer_labels)
		{
			if (has_debug_utils_ext)
			{
				_ext_functions._vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance, "vkCmdBeginDebugUtilsLabelEXT"));
				_ext_functions._vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance, "vkCmdEndDebugUtilsLabelEXT"));
				_ext_functions._vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance, "vkCmdInsertDebugUtilsLabelEXT"));
			}
			else
			{
				_options.enable_command_buffer_labels = false;
			}
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
			.vulkanApiVersion = VK_API_VERSION_1_3,
		};
		vmaCreateAllocator(&alloc_ci, &_allocator);
		//_staging_pool = std::make_unique<StagingPool>(this, _allocator);
	}

	void VkApplication::nameVkObjectIFP(VkDebugUtilsObjectNameInfoEXT const& object_to_name)
	{
		if (_options.enable_object_naming)
		{
			assertm(_ext_functions._vkSetDebugUtilsObjectNameEXT != nullptr, "VkFunction not loaded!");
			VK_CHECK(_ext_functions._vkSetDebugUtilsObjectNameEXT(_device, &object_to_name), "Failed to name an object.");
		}
	}

	void VkApplication::nameVkObjectIFP(VkObjectType type, uint64_t handle, std::string_view const& name)
	{
		VkDebugUtilsObjectNameInfoEXT doni{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = type,
			.objectHandle = handle, 
			.pObjectName = name.data(),
		};
		nameVkObjectIFP(doni);
	}

	void VkApplication::queryDescriptorBindingOptions()
	{
		_descriptor_binding_options.use_push_descriptors = false;//hasDeviceExtension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
		_descriptor_binding_options.set_bindings.resize(_device_props.props2.properties.limits.maxBoundDescriptorSets);
		_desc_set_layout_caches.resize(_device_props.props2.properties.limits.maxBoundDescriptorSets);
		for (size_t i = 0; i < _descriptor_binding_options.set_bindings.size(); ++i)
		{
			_descriptor_binding_options.set_bindings[i] = BindingIndex{.set = static_cast<uint32_t>(i), .binding = 0};
		}
		_descriptor_binding_options.merge_module_and_shader = _descriptor_binding_options.set_bindings.size() <= 4;
		_descriptor_binding_options.shader_set = _descriptor_binding_options.set_bindings[static_cast<uint32_t>(DescriptorSetName::shader)].set;

	}

	std::shared_ptr<DescriptorSetLayoutInstance> VkApplication::getEmptyDescSetLayout()
	{
		std::unique_lock lock(_mutex);
		if (!_empty_set_layout)
		{
			_empty_set_layout = std::make_shared<DescriptorSetLayoutInstance>(DescriptorSetLayoutInstance::CI{
				.app = this,
				.name = "EmptyDescSetLayout",
			});
		}
		return _empty_set_layout;
	}

	void VkApplication::initSDL()
	{
		uint32_t init = 0;
		init |= SDL_INIT_VIDEO;
		init |= SDL_INIT_EVENTS;
		init |= SDL_INIT_GAMECONTROLLER;
		if (SDL_Init(init) < 0)
		{
			std::cout << SDL_GetError() << std::endl;
			exit(-1);
		}
		SDL_Vulkan_LoadLibrary(nullptr);
	}

	VkApplication::VkApplication(CreateInfo const& ci) :
		_name(ci.name)
	{
		if(ci.args.is_used("--name"))
		{
			std::string args_name = ci.args.get<std::string>("--name");	
			_name = std::move(args_name);
		}

		auto intToBool = [](int i)
		{
			return i != 0;
		};

		_options = Options{
			.enable_validation = intToBool(ci.args.get<int>("--validation")),
			.enable_object_naming = intToBool(ci.args.get<int>("--name_vk_objects")),
			.enable_command_buffer_labels = intToBool(ci.args.get<int>("--cmd_labels")),
			.gpu_id = static_cast<uint32_t>(ci.args.get<int>("--gpu")),
		};

		bool mt = true;
		size_t n_threads = 0;
		bool log = true;

		std::string arg_helper_threads = ci.args.get<std::string>("--helper_threads");
		if (arg_helper_threads == "all")
		{
			n_threads = std::thread::hardware_concurrency();
		}
		else if (arg_helper_threads == "none")
		{
			mt = false;
		}
		else
		{
			int n = std::atoi(arg_helper_threads.c_str());
			if (n < 0)
			{
				n = std::thread::hardware_concurrency() - n;
			}
			n_threads = n;
		}

		_thread_pool = std::unique_ptr<DelayedTaskExecutor>(DelayedTaskExecutor::MakeNew(DelayedTaskExecutor::MakeInfo{
			.multi_thread = mt,
			.n_threads = n_threads,
			.log_actions = log,
		}));
	}

	void VkApplication::init()
	{
		loadMountingPoints();
		initSDL();
		preChecks();

		requestFeatures(_requested_features);

		initInstance(_name);
		initValidLayers();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createCommandPools();

		_sampler_library = std::make_unique<SamplerLibrary>(SamplerLibrary::CI{
			.app = this,
			.name = "SamplerLibrary",
		});

		_texture_file_cache = std::make_unique<TextureFileCache>(TextureFileCache::CI{
			.app = this,
			.name = "TextureFileCache",
		});

		_prebuilt_transfer_commands = std::make_unique<PrebuilTransferCommands>(this);

	}

	void VkApplication::cleanup()
	{
		_prebuilt_transfer_commands.reset();
		_texture_file_cache.reset();
		_sampler_library.reset();;

		_empty_set_layout = nullptr;

		_pools.graphics = nullptr;
		_pools.transfer = nullptr;
		_pools.compute = nullptr;

		_desc_set_layout_caches.clear();

		//_staging_pool = nullptr;
		vmaDestroyAllocator(_allocator);

		vkDestroyDevice(_device, nullptr);

		if (_options.enable_validation)
		{
			DestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
		}

		vkDestroyInstance(_instance, nullptr);

		SDL_Vulkan_UnloadLibrary();
		SDL_Quit();
		
		{
			if (_thread_pool->waitAll())
			{

			}
			else
			{
				
			}
			_thread_pool = nullptr;
		}
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

	void VkApplication::loadMountingPoints()
	{
		const std::filesystem::path current_path = std::filesystem::current_path();
		const std::filesystem::path exe_path = GetCurrentExecutableAbsolutePath();
		const std::filesystem::path exe_folder = exe_path.parent_path();

		// Default values
		_mounting_points["exec"] = exe_folder.string();
		_mounting_points["ShaderLib"] = exe_folder.string() + "/ShaderLib/";
		
		_mounting_points["gen"] = exe_folder.string() + "/gen/";

		std::filesystem::path mp_file_path = exe_folder / "MountingPoints.txt";

		if (std::filesystem::exists(mp_file_path))
		{
			try 
			{
				const std::string file = ReadFileToString(mp_file_path);
				const std::vector<std::string> parsed = [&]()
				{
					std::vector<std::string> res;
					size_t it = 0;
					while (it < file.size())
					{
						size_t open_dq = file.find('\"', it);
						size_t close_dq = file.find('\"', open_dq + 1);
						if (open_dq < file.size() && close_dq < file.size())
						{
							res.push_back(file.substr(open_dq + 1, close_dq - open_dq - 1));
							it = close_dq + 1;
						}
						else
						{
							break;
						}
					}
					// Must be even
					if ((res.size() % 2) == 1)
					{

					}
					return res;
				}();

				for (size_t i = 0; i < parsed.size(); i += 2)
				{
					std::string const& key = parsed[i];
					std::string const& value = parsed[i + 1];
					std::filesystem::path path = value;
					if (path.is_relative())
					{
						path = exe_folder / path;
					}
					_mounting_points[key] = path.string();
				}

				// Special case when developping:
				if(_mounting_points.contains("DevProjectsFolder"))
				{
					if (!_mounting_points.contains("ProjectShaders"))
					{
						_mounting_points["ProjectShaders"] = _mounting_points["DevProjectsFolder"] + GetProjectName() + "/Shaders/"s;
					}
				}
			}
			catch (std::exception const& e)
			{
				std::cout << "Could not find the Mounting Points file. Using Executable folder in place." << std::endl;
			};
		}
		if (!_mounting_points.contains("ProjectShaders"))
		{
			_mounting_points["ProjectShaders"] = exe_folder.string() + "/Shaders/";
		}
	}
}
