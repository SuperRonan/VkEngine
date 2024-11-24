
#include <vkl/App/VkApplication.hpp>

#include <vkl/VkObjects/CommandBuffer.hpp>
#include <vkl/VkObjects/Queue.hpp>
#include <vkl/VkObjects/DescriptorSetLayout.hpp>
#include <vkl/VkObjects/VulkanExtensionsSet.hpp>

#include <vkl/Execution/SamplerLibrary.hpp>

#include <vkl/Rendering/TextureFromFile.hpp>

#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <that/IO/File.hpp>
#include <vkl/IO/Logging.hpp> 

#include <argparse/argparse.hpp>

#include <ShaderLib/Vulkan/ShaderAtomicFlags.h>

#include <slang/slang.h>

#include <exception>
#include <set>
#include <limits>
#include <fstream>
#include <span>


namespace vkl
{

	void VkApplication::FillArgs(argparse::ArgumentParser& args)
	{
		args.add_argument("--name")
			.help("Name of the Application")
		;

		int default_validation = 0;
		int default_cmd_labels = 0;
		int default_verbosity = 3;

#if VKL_BUILD_ANY_DEBUG
		default_validation = 1;
		default_cmd_labels = 1;
		default_verbosity = 2;
#endif
#if VKL_BUILD_RELEASE_WITH_DEBUG_INFO
		default_validation = 0;
		default_cmd_labels = 1;
		default_verbosity = 1;
#elif VKL_BUILD_RELEASE
		default_validation = 0;
		default_cmd_labels = 0;
		default_verbosity = 1;
#endif
		int default_name_vk_objects = std::max(default_validation, default_cmd_labels);

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

		args.add_argument("--image_layout")
			.help("Select which image layout to use, possible values are: 'specific', 'general', 'auto' or a bit mask per usage (0 for specfic, 1 for general)")
			.default_value("specific")
		;

		args.add_argument("--verbosity")
			.help("Set the console verbosity level (int)")
			.default_value(default_verbosity)
			.scan<'d', int>()
		;

		args.add_argument("--dump_shader_source")
			.help("Enable shaders source dump in the gen folder")
			.scan<'d', int>()
			.default_value(0)
		;
		
		args.add_argument("--dump_spv")
			.help("Enable shaders SPIR-V binary dump in the gen folder")
			.scan<'d', int>()
			.default_value(0)
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

	std::set<std::string_view> VkApplication::getValidLayers()
	{
		return {
			"VK_LAYER_KHRONOS_validation"
		};
	}

	std::set<std::string_view> VkApplication::getDeviceExtensions()
	{
		return {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
			VK_KHR_PRESENT_ID_EXTENSION_NAME,
			VK_KHR_PRESENT_WAIT_EXTENSION_NAME,

			VK_EXT_SUBPASS_MERGE_FEEDBACK_EXTENSION_NAME,

			VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
			VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,
			VK_EXT_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME,

			VK_NV_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME,
			VK_NV_RAY_TRACING_MOTION_BLUR_EXTENSION_NAME,

			VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME,
		};
	}

	void VkApplication::requestFeatures(VulkanFeatures& features)
	{
		const VkBool32 t = VK_TRUE;
		const VkBool32 f = VK_FALSE;
		
		features.features_13.synchronization2 = t;
		features.swapchain_maintenance1_ext.swapchainMaintenance1 = t;
		features.present_id_khr.presentId = t;

		features.features2.features.geometryShader = t;

		features.features2.features.samplerAnisotropy = t;
		features.features_12.samplerMirrorClampToEdge = t;

		features.features2.features.multiDrawIndirect = t;
		features.features_11.shaderDrawParameters = t;


		features.features_12.shaderInt8 = t;
		features.features_12.shaderFloat16 = t;
		features.features_11.storageBuffer16BitAccess = t;
	
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

		features.subpass_merge_feedback.subpassMergeFeedback = t;

		features.line_raster_ext.bresenhamLines = t;
		features.line_raster_ext.stippledBresenhamLines = t;

		features.index_uint8_ext.indexTypeUint8 = t;

		features.mesh_shader_ext.meshShader = t;
		features.mesh_shader_ext.taskShader = t;
		features.mesh_shader_ext.multiviewMeshShader = t;
		//features.mesh_shader_ext.primitiveFragmentShadingRateMeshShader = t;

		features.robustness2_ext.nullDescriptor = t;

		features.features_13.dynamicRendering = t;

		features.features_12.bufferDeviceAddress = t;
		features.acceleration_structure_khr.accelerationStructure = t;
		features.acceleration_structure_khr.descriptorBindingAccelerationStructureUpdateAfterBind = t;

		features.ray_tracing_pipeline_khr.rayTracingPipeline = t;
		features.ray_tracing_pipeline_khr.rayTraversalPrimitiveCulling = t;

		features.ray_query_khr.rayQuery = t;

		features.ray_tracing_position_fetch_khr.rayTracingPositionFetch = t;

		features.ray_tracing_validation_nv.rayTracingValidation = t;
	}

	std::set<std::string_view> VkApplication::getInstanceExtensions()
	{
		uint32_t sdl_ext_count = 0;
		const char* const * sdl_extensions_ptr = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
		std::set<std::string_view> res(sdl_extensions_ptr, sdl_extensions_ptr + sdl_ext_count);
		
		if (_options.enable_validation || _options.enable_object_naming || _options.enable_command_buffer_labels)
		{
			res.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		res.insert(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
		res.insert(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
		res.insert(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
		
		return res;
	}

	VkApplication::DesiredQueuesInfo VkApplication::getDesiredQueuesInfo()
	{
		DesiredQueuesInfo res;
		res.need_presentation = false;
		res.queues.push_back(DesiredQueueInfo{
			.flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
			.priority = 1,
			.required = true,
			.name = "MainQueue",
		});
		return res;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
	{
		const VkApplication * app = reinterpret_cast<const VkApplication*>(user_data);
		bool ignore = message_severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		// There is a bug in the current SDK's VLL which emits this incorrect error
		std::string_view message = callback_data->pMessage;
		// Bug in SDK 1.3.283
		ignore |= ((message.find("VUID-vkCmdTraceRaysKHR-None-08608") != std::string_view::npos) && (message.find("VK_DYNAMIC_STATE_VIEWPORT") != std::string_view::npos));
		// Bug in SDK 1.3.290
		ignore |= ((message.find("VUID-VkShaderModuleCreateInfo-pCode-08737") != std::string_view::npos) && (message.find("Expected Image to have the same type as Result Type Image") != std::string_view::npos));
		if (!ignore)
		{
			bool bp = message_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			Logger::Options tag = Logger::Options::None;
			Logger::Options verbose = Logger::Options::None;
			switch (message_severity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				tag = Logger::Options::TagInfo3;
				verbose = Logger::Options::VerbosityLeastImportant;
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				tag = Logger::Options::TagInfo2;
				verbose = Logger::Options::VerbosityMedium;
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				tag = Logger::Options::TagWarning;
			break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				tag = Logger::Options::TagError;
			break;
			}

			switch (message_type)
			{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			
			break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			
			break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			
			break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			
			break;
			}
			const Logger::Options options = tag | verbose;
			app->logger()(callback_data->pMessage, options);
			if (bp)
			{
 				VKL_BREAKPOINT_HANDLE;
			}
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

		bool res = std::all_of(valid_layers.cbegin(), valid_layers.cend(), [&available_layers](std::string_view layer_name)
			{
				return available_layers.cend() != std::find_if(available_layers.cbegin(), available_layers.cend(), [&layer_name](VkLayerProperties const& layer_prop)
					{
						return strcmp(layer_name.data(), layer_prop.layerName) == 0;
					});
			});

		return res;
	}

	void VkApplication::preChecks()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		//VK_LOG << extensionCount << " extensions supported\n";

		std::vector<VkExtensionProperties> available_extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, available_extensions.data());

		//for (const auto& e : available_extensions)
		//{
		//	VK_LOG << '\t' << e.extensionName << '\n';
		//}


		if (_options.enable_validation)
		{
			if (!checkValidLayerSupport())
			{
				_logger("Missing Validation Layer!", Logger::Options::TagError);
				_options.enable_validation = false;
			}
			_logger("Found All Required Validation Layers!", Logger::Options::TagSuccess);
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

		auto desired_extensions = getInstanceExtensions();
		_instance_extensions = std::make_unique<VulkanExtensionsSet>(desired_extensions, static_cast<VkPhysicalDevice>(VK_NULL_HANDLE), nullptr);
		// TODO check that extensions are available
		VkInstanceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
			.enabledExtensionCount = _instance_extensions->pExts().size32(),
			.ppEnabledExtensionNames = _instance_extensions->pExts().data(),
		};

		VkDebugUtilsMessengerCreateInfoEXT instance_debug_create_info{};

		const auto valid_layers = getValidLayers();
		MyVector<const char*> p_valid_layers = flatten(valid_layers);

		if (_options.enable_validation)
		{
			create_info.enabledLayerCount = p_valid_layers.size32();
			create_info.ppEnabledLayerNames = p_valid_layers.data();

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
			.pUserData = reinterpret_cast<void*>(const_cast<VkApplication*>(this)),
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

	VkQueueFlags VkApplication::DesiredQueuesInfo::totalFlags()const
	{
		VkQueueFlags res = 0;
		for (const auto& q : queues)
		{
			res |= q.flags;
		}
		return res;
	}

	VkApplication::DeviceCandidateQueues VkApplication::findQueueFamilies(VkPhysicalDevice device, DesiredQueuesInfo const& desired)
	{
		DeviceCandidateQueues res;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

		//VK_LOG << "Found " << queue_family_count << " queue(s).\n";

		MyVector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		res.queues.resize(queue_family_count);
		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			res.queues[i].props = queue_families[i];
			res.total_flags |= queue_families[i].queueFlags;
		}

		res.desired_to_info.resize(desired.queues.size());
		for (size_t j = 0; j < desired.queues.size(); ++j)
		{
			const auto & desired_queue = desired.queues[j];
			assert(desired_queue.flags != 0);
			uint32_t best_matching_id = uint32_t(-1);
			uint32_t best_matching_flags = std::numeric_limits<uint32_t>::max();
			const uint32_t expected_flags_count = std::popcount(desired_queue.flags);
			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				const VkQueueFlags inter = desired_queue.flags & queue_families[i].queueFlags;
				const uint32_t count = std::popcount(inter);
				const bool enough = count >= expected_flags_count;
				if (enough && (count < best_matching_flags)) // Find the queue with just enough flags
				{
					best_matching_id = i;
					best_matching_flags = count;
				}
			}
			res.desired_to_info[j].family = best_matching_id;
			if (best_matching_id < res.queues.size32())
			{
				res.desired_to_info[j].index = res.queues[best_matching_id].priorities.size32();
				res.queues[best_matching_id].priorities.push_back(desired_queue.priority);
				res.queues[best_matching_id].names.push_back(desired_queue.name);
			}
			else if(desired_queue.required)
			{
				res.all_required &= false;
			}
		}
		
		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			const uint32_t capacity = res.queues[i].props.queueCount;
			const uint32_t wanted = res.queues[i].priorities.size32();
			if (wanted > capacity)
			{
				// TODO merge queues
				NOT_YET_IMPLEMENTED;
			}
		}
		if (desired.need_presentation)
		{
			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				bool can_present = false;
#if _WINDOWS
				can_present = vkGetPhysicalDeviceWin32PresentationSupportKHR(device, i);
#endif
				if (can_present)
				{
					res.present_queues.push_back(i);
				}
			}
		}
		return res;
	}

	bool VkApplication::isDeviceSuitable(CandidatePhysicalDevice & candidate, DesiredDeviceInfo const& desired) {
		bool res = true;

		VkBool32 present_support = false;
		DeviceCandidateQueues candidate_queues = findQueueFamilies(candidate.device, desired.queues);
		candidate.queues = candidate_queues;

		if (desired.queues.need_presentation && candidate_queues.present_queues.empty())
		{
			res = false;
		}

		const VkQueueFlags total_queue_desired_flags = desired.queues.totalFlags();
		const VkQueueFlags total_queue_candidate_falgs = candidate.queues.total_flags;
		res &= ((total_queue_desired_flags & total_queue_candidate_falgs) !=0);
		res &= candidate.queues.all_required;

		// Check for known necessary features
		//const uint32_t min_version = VK_MAKE_VERSION(1, 3, 0);
		const uint32_t min_version = VK_MAKE_API_VERSION(0, 1, 3, 0);
		res &= candidate.props.props2.properties.apiVersion >= min_version;

		res &= candidate.features.features_13.synchronization2 != VK_FALSE;

		return res;
	}

	int64_t VkApplication::ratePhysicalDevice(VkPhysicalDevice device, DesiredDeviceInfo const& desired) {
		int64_t res = 0;
		
		CandidatePhysicalDevice candidate;
		candidate.device = device;
		// TODO add pLayerName
		candidate.extensions = VulkanExtensionsSet(device);
		const uint32_t version = vkGetPhysicalDeviceAPIVersion(device);

		std::function<bool(std::string_view)> filter_extensions = [&](std::string_view ext_name){return candidate.extensions.contains(ext_name);};

		vkGetPhysicalDeviceProperties2(device, &candidate.props.link(version, filter_extensions));
		vkGetPhysicalDeviceFeatures2(device, &candidate.features.link(version, filter_extensions));

		bool suitable = isDeviceSuitable(candidate, desired);
		if (!suitable)
		{
			return std::numeric_limits<int64_t>::min();
		}
		
		int64_t discrete_multiplicator = (candidate.props.props2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 2 : 1;

		res = 1;

		uint32_t ext_count = 0;
		for (std::string_view const& desired_ext_name : desired.extensions)
		{
			if (candidate.extensions.contains(desired_ext_name))
			{
				ext_count += 1;
			}
		}

		uint32_t num_features = (candidate.features && desired.features).count();

		res = (1 + ext_count + num_features) * discrete_multiplicator;

		return res;
	}

	void VkApplication::pickPhysicalDevice()
	{
		DesiredDeviceInfo desired;
		desired.extensions = getDeviceExtensions();
		requestFeatures(desired.features);
		desired.queues = getDesiredQueuesInfo();

		uint32_t physical_device_count = 0;
		vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr);

		if (physical_device_count == 0)
		{
			_logger("No physical device found!", Logger::Options::TagFatalError);
			throw std::runtime_error("No physical device found!");
		}

		std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
		vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.data());

		std::stringstream ss;
		ss << "Found " << physical_device_count << " physical device(s):\n";
		int64_t best_score = std::numeric_limits<int64_t>::min();
		size_t best_index = 0;
		for (size_t i = 0; i < physical_devices.size(); ++i)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(physical_devices[i], &props);
			int64_t rating = ratePhysicalDevice(physical_devices[i], desired);
			ss << " - " << i << ": " << props.deviceName << ", Rated: " << rating;
			if (i != (physical_devices.size() - 1))
			{
				ss << "\n";
			}
			if (rating > best_score)
			{
				best_index = i;
				best_score = rating;
			}
		}
		_logger(ss.str(), Logger::Options::VerbosityMostImportant | Logger::Options::TagInfo3);

		if (_options.gpu_id < physical_device_count)
		{
			best_index = _options.gpu_id;
		}
		_physical_device = physical_devices[best_index];

		_device_extensions = std::make_unique<VulkanExtensionsSet>(desired.extensions, _physical_device);
		VkPhysicalDeviceProperties2 & physical_device_props = _device_props.link(
			vkGetPhysicalDeviceAPIVersion(_physical_device),
			[this](std::string_view ext_name) {return _device_extensions->contains(ext_name); }
		);
		vkGetPhysicalDeviceProperties2(_physical_device, &physical_device_props);

		_logger(std::format("Using {} as physical device", _device_props.props2.properties.deviceName), Logger::Options::VerbosityMostImportant | Logger::Options::TagSuccess);
	}

	void VkApplication::createLogicalDevice()
	{
		const DesiredQueuesInfo desired_queues = getDesiredQueuesInfo();
		DeviceCandidateQueues device_queues = findQueueFamilies(_physical_device, desired_queues);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		queue_create_infos.reserve(device_queues.queues.size());
		
		for (size_t i=0; i < device_queues.queues.size(); ++i)
		{
			if (!device_queues.queues[i].priorities.empty())
			{
				queue_create_infos.push_back(VkDeviceQueueCreateInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueFamilyIndex = static_cast<uint32_t>(i),
					.queueCount = device_queues.queues[i].priorities.size32(),
					.pQueuePriorities = device_queues.queues[i].priorities.data(),
				});
			}
		}

		auto filter_extensions = [this](std::string_view ext_name){return _device_extensions->contains(ext_name); };

		VulkanFeatures exposed_device_features;
		vkGetPhysicalDeviceFeatures2(_physical_device, &exposed_device_features.link(vkGetPhysicalDeviceAPIVersion(_physical_device), filter_extensions));

		VulkanFeatures requested_features;
		requestFeatures(requested_features);
		_available_features = filterFeatures(requested_features, exposed_device_features);

		VkPhysicalDeviceFeatures2 features2 = _available_features.link(vkGetPhysicalDeviceAPIVersion(_physical_device), filter_extensions);

		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext = &features2;
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
		device_create_info.queueCreateInfoCount = queue_create_infos.size();
		device_create_info.pEnabledFeatures = nullptr;

		device_create_info.enabledExtensionCount = _device_extensions->pExts().size32();
		device_create_info.ppEnabledExtensionNames = _device_extensions->pExts().data();

		const auto valid_layers = getValidLayers();
		MyVector<const char*> p_valid_layers = flatten(valid_layers);
		if (_options.enable_validation)
		{
			device_create_info.enabledLayerCount = p_valid_layers.size32();
			device_create_info.ppEnabledLayerNames = p_valid_layers.data();
		}
		else
		{
			device_create_info.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(_physical_device, &device_create_info, nullptr, &_device), "Failed to create the logical device!");

		loadExtFunctionsPtr();
		{
			_queues_by_family.resize(device_queues.queues.size());
			for (uint32_t i = 0; i < _queues_by_family.size32(); ++i)
			{
				_queues_by_family[i].resize(device_queues.queues[i].priorities.size());
				for (uint32_t j = 0; j < _queues_by_family[i].size32(); ++j)
				{
					_queues_by_family[i][j] = std::make_shared<Queue>(Queue::CI{
						.app = this,
						.name = device_queues.queues[i].names[j],
						.flags = 0,
						.family_index = i,
						.index = j,
						.properties = device_queues.queues[i].props,
						.priority = device_queues.queues[i].priorities[j],
					});
				}
			}
			_queues_by_desired = device_queues.desired_to_info;
		}


		_options.prefer_render_pass_with_dynamic_rendering &= (_available_features.features_13.dynamicRendering) != VK_FALSE;
		_options.query_render_pass_creation_feedback &= (_available_features.subpass_merge_feedback.subpassMergeFeedback) != VK_FALSE;
		_options.render_pass_disallow_merging &= (_available_features.subpass_merge_feedback.subpassMergeFeedback) != VK_FALSE;

		queryDescriptorBindingOptions();
	}

	void VkApplication::loadExtFunctionsPtr()
	{
		if (_available_features.mesh_shader_ext.meshShader)
		{
			_ext_functions._vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT"));
			_ext_functions._vkCmdDrawMeshTasksIndirectEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksIndirectEXT"));
			_ext_functions._vkCmdDrawMeshTasksIndirectCountEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectCountEXT>(vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksIndirectCountEXT"));
		}

		const bool has_debug_utils_ext = instanceExtensions().contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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

		if (_available_features.acceleration_structure_khr.accelerationStructure)
		{
			_ext_functions._vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCreateAccelerationStructureKHR"));
			_ext_functions._vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkDestroyAccelerationStructureKHR"));
			_ext_functions._vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkCmdBuildAccelerationStructuresKHR"));
			_ext_functions._vkCmdBuildAccelerationStructuresIndirectKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresIndirectKHR>(vkGetDeviceProcAddr(_device, "vkCmdBuildAccelerationStructuresIndirectKHR"));
			_ext_functions._vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkBuildAccelerationStructuresKHR"));
			_ext_functions._vkCopyAccelerationStructureKHR = reinterpret_cast<PFN_vkCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCopyAccelerationStructureKHR"));
			_ext_functions._vkCopyAccelerationStructureToMemoryKHR = reinterpret_cast<PFN_vkCopyAccelerationStructureToMemoryKHR>(vkGetDeviceProcAddr(_device, "vkCopyAccelerationStructureToMemoryKHR"));
			_ext_functions._vkCopyMemoryToAccelerationStructureKHR = reinterpret_cast<PFN_vkCopyMemoryToAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCopyMemoryToAccelerationStructureKHR"));
			_ext_functions._vkWriteAccelerationStructuresPropertiesKHR = reinterpret_cast<PFN_vkWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(_device, "vkWriteAccelerationStructuresPropertiesKHR"));
			_ext_functions._vkCmdCopyAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCmdCopyAccelerationStructureKHR"));
			_ext_functions._vkCmdCopyAccelerationStructureToMemoryKHR = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureToMemoryKHR>(vkGetDeviceProcAddr(_device, "vkCmdCopyAccelerationStructureToMemoryKHR"));
			_ext_functions._vkCmdCopyMemoryToAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdCopyMemoryToAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCmdCopyMemoryToAccelerationStructureKHR"));
			_ext_functions._vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureDeviceAddressKHR"));
			_ext_functions._vkCmdWriteAccelerationStructuresPropertiesKHR = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(_device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
			_ext_functions._vkGetDeviceAccelerationStructureCompatibilityKHR = reinterpret_cast<PFN_vkGetDeviceAccelerationStructureCompatibilityKHR>(vkGetDeviceProcAddr(_device, "vkGetDeviceAccelerationStructureCompatibilityKHR"));
			_ext_functions._vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureBuildSizesKHR"));
		}

		if (_available_features.ray_tracing_pipeline_khr.rayTracingPipeline)
		{
			_ext_functions._vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_device, "vkCmdTraceRaysKHR"));
			_ext_functions._vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_device, "vkCreateRayTracingPipelinesKHR"));
			_ext_functions._vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR"));
			_ext_functions._vkCmdTraceRaysIndirectKHR = reinterpret_cast<PFN_vkCmdTraceRaysIndirectKHR>(vkGetDeviceProcAddr(_device, "vkCmdTraceRaysIndirectKHR"));
			_ext_functions._vkGetRayTracingShaderGroupStackSizeKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupStackSizeKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingShaderGroupStackSizeKHR"));
			_ext_functions._vkCmdSetRayTracingPipelineStackSizeKHR = reinterpret_cast<PFN_vkCmdSetRayTracingPipelineStackSizeKHR>(vkGetDeviceProcAddr(_device, "vkCmdSetRayTracingPipelineStackSizeKHR"));
			_ext_functions._vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingShaderGroupHandlesKHR"));
		}

		if (_available_features.present_wait_khr.presentWait)
		{
			_ext_functions._vkWaitForPresentKHR = reinterpret_cast<PFN_vkWaitForPresentKHR>(vkGetDeviceProcAddr(_device, "vkWaitForPresentKHR"));
		}
	}


	void VkApplication::createCommandPools()
	{
		const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		_command_pools.resize(_queues_by_family.size());
		for (uint32_t i = 0; i < _queues_by_family.size32(); ++i)
		{
			if (!_queues_by_family[i].empty())
			{
				_command_pools[i] = std::make_shared<CommandPool>(CommandPool::CI{
					.app = this,
					.name = "Command_Pool_" + std::to_string(i),
					.queue_family = i,
					.flags = flags,
				});
			}
			else
			{
				_command_pools[i] = nullptr;
			}
		}
	}

	void VkApplication::createAllocator()
	{
		VmaAllocatorCreateFlags flags = 0;
		if (availableFeatures().features_12.bufferDeviceAddress)
		{
			flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}
		VmaAllocatorCreateInfo alloc_ci{
			.flags = flags,
			.physicalDevice = _physical_device,
			.device = _device,
			.instance = _instance,
			.vulkanApiVersion = VK_API_VERSION_1_3,
		};
		vmaCreateAllocator(&alloc_ci, &_allocator);
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

	void VkApplication::fillCommonShaderDefinitions()
	{
		VulkanFeatures const& features = availableFeatures();
		VulkanDeviceProps const& props = deviceProperties();

		const auto tod = [](VkBool32 b) {
			return b ? "1" : "0";
			};

		DefinitionsMap & defs = _common_shader_definitions;

		defs.setDefinition("SHADER_DEVICE_TYPE", std::to_string(props.props2.properties.deviceType));
		defs.setDefinition("SHADER_DEVICE_ID", std::to_string(props.props2.properties.deviceID));
		defs.setDefinition("SHADER_VENDOR_ID", std::to_string(props.props2.properties.vendorID));
		defs.setDefinition("SHADER_VK_API_VERSION", std::to_string(props.props2.properties.apiVersion));
		defs.setDefinition("SHADER_DRIVER_VERSION", std::to_string(props.props2.properties.driverVersion));
		defs.setDefinition("SHADER_DRIVER_ID", std::to_string(props.props_12.driverID));

		defs.setDefinition("SHADER_MAX_BOUND_DESCRIPTOR_SETS", std::to_string(props.props2.properties.limits.maxBoundDescriptorSets));
		defs.setDefinition("SHADER_MAX_PUSH_CONSTANT_SIZE", std::to_string(props.props2.properties.limits.maxPushConstantsSize));

		defs.setDefinition("SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT", tod(features.features2.features.shaderStorageImageReadWithoutFormat));
		defs.setDefinition("SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT", tod(features.features2.features.shaderStorageImageWriteWithoutFormat));
		defs.setDefinition("SHADER_IMAGE_GATHER_EXTENDED", tod(features.features2.features.shaderImageGatherExtended));
		defs.setDefinition("SHADER_STORAGE_IMAGE_EXTENDED_FORMATS", tod(features.features2.features.shaderStorageImageExtendedFormats));
		defs.setDefinition("SHADER_STORAGE_IMAGE_MULTISAMPLE", tod(features.features2.features.shaderStorageImageMultisample));

		defs.setDefinition("SHADER_CLIP_DISTANCE_AVAILABLE", tod(features.features2.features.shaderClipDistance));
		defs.setDefinition("SHADER_CULL_DISTANCE_AVAILABLE", tod(features.features2.features.shaderCullDistance));
		defs.setDefinition("SHADER_FP64_AVAILABLE", tod(features.features2.features.shaderFloat64));
		defs.setDefinition("SHADER_INT64_AVAILABLE", tod(features.features2.features.shaderInt64));
		defs.setDefinition("SHADER_INT16_AVAILABLE", tod(features.features2.features.shaderInt16));

		defs.setDefinition("SHADER_POINT_SIZE_RANGE_MIN", std::to_string(props.props2.properties.limits.pointSizeRange[0]));
		defs.setDefinition("SHADER_POINT_SIZE_RANGE_MAX", std::to_string(props.props2.properties.limits.pointSizeRange[1]));
		defs.setDefinition("SHADER_POINT_SIZE_GRANULARITY", std::to_string(props.props2.properties.limits.pointSizeGranularity));
		defs.setDefinition("SHADER_LINE_WIDTH_RANGE_MIN", std::to_string(props.props2.properties.limits.lineWidthRange[0]));
		defs.setDefinition("SHADER_LINE_WIDTH_RANGE_MAX", std::to_string(props.props2.properties.limits.lineWidthRange[1]));
		defs.setDefinition("SHADER_LINE_WIDTH_GRANULARITY", std::to_string(props.props2.properties.limits.lineWidthGranularity));

		defs.setDefinition("SHADER_MAX_FRAGMENT_OUTPUT_ATTACHMENTS", std::to_string(props.props2.properties.limits.maxFragmentOutputAttachments));

		defs.setDefinition("SHADER_GEOMETRY_AVAILABLE", tod(features.features2.features.geometryShader));
		defs.setDefinition("SHADER_MAX_GEOMETRY_INPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryInputComponents));
		defs.setDefinition("SHADER_MAX_GEOMETRY_OUTPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryOutputComponents));
		defs.setDefinition("SHADER_MAX_GEOMETRY_OUTPUT_VERTICES", std::to_string(props.props2.properties.limits.maxGeometryOutputVertices));
		defs.setDefinition("SHADER_MAX_GEOMETRY_SHADER_INVOCATIONS", std::to_string(props.props2.properties.limits.maxGeometryShaderInvocations));
		defs.setDefinition("SHADER_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS", std::to_string(props.props2.properties.limits.maxGeometryTotalOutputComponents));

		defs.setDefinition("SHADER_TESSELATION_AVAILABLE", tod(features.features2.features.tessellationShader));

		defs.setDefinition("SHADER_FP16_AVAILABLE", tod(features.features_12.shaderFloat16));
		defs.setDefinition("SHADER_SSBO_16BITS_ACCESS", tod(features.features_11.storageBuffer16BitAccess));

		defs.setDefinition("SHADER_MAX_COMPUTE_WORKGROUP_SUBGROUPS", std::to_string(props.props_13.maxComputeWorkgroupSubgroups));
		defs.setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE", std::to_string(props.props2.properties.limits.maxComputeWorkGroupInvocations));
		defs.setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_X", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[0]));
		defs.setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_Y", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[1]));
		defs.setDefinition("SHADER_MAX_COMPUTE_LOCAL_SIZE_Z", std::to_string(props.props2.properties.limits.maxComputeWorkGroupSize[2]));

		defs.setDefinition("SHADER_RAY_QUERY_AVAILABLE", tod(features.ray_query_khr.rayQuery));
		defs.setDefinition("SHADER_RAY_TRACING_AVAILABLE", tod(features.ray_tracing_pipeline_khr.rayTracingPipeline));
		defs.setDefinition("SHADER_RAY_TRACING_MAX_RAY_HIT_ATTRIBUTE_SIZE", std::to_string(props.ray_tracing_pipeline_khr.maxRayHitAttributeSize));
		defs.setDefinition("SHADER_RAY_TRACING_MAX_RAY_RECURSION_DEPTH", std::to_string(props.ray_tracing_pipeline_khr.maxRayRecursionDepth));
		defs.setDefinition("SHADER_RAY_TRACING_INVOCATION_REORDER_AVAILABLE", tod(features.ray_tracing_invocation_reorder_nv.rayTracingInvocationReorder));
		defs.setDefinition("SHADER_RAY_TRACING_INVOCATION_REORDER_REORDERING_HINT", std::to_string(props.ray_tracing_invocation_reorder_nv.rayTracingInvocationReorderReorderingHint));
		defs.setDefinition("SHADER_RAY_TRACING_POSITION_FETCH_AVAILABLE", tod(features.ray_tracing_position_fetch_khr.rayTracingPositionFetch));

		defs.setDefinition("SHADER_SUBGROUP_SIZE", std::to_string(props.props_11.subgroupSize));
		defs.setDefinition("SHADER_SUBGROUP_SUPPORTED_OPERATIONS", std::to_string(props.props_11.subgroupSupportedOperations));
		defs.setDefinition("SHADER_SUBGROUP_SUPPORTED_STAGES", std::to_string(props.props_11.subgroupSupportedStages));

		defs.setDefinition("SHADER_MAX_TASK_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxTaskWorkGroupInvocations));
		defs.setDefinition("SHADER_MAX_PREFERED_TASK_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxPreferredTaskWorkGroupInvocations));
		defs.setDefinition("SHADER_MAX_MESH_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxMeshWorkGroupInvocations));
		defs.setDefinition("SHADER_MAX_PREFERED_MESH_LOCAL_SIZE", std::to_string(props.mesh_shader_ext.maxPreferredMeshWorkGroupInvocations));
		defs.setDefinition("SHADER_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY", std::to_string(props.mesh_shader_ext.meshOutputPerPrimitiveGranularity));
		defs.setDefinition("SHADER_MESH_OUTPUT_PER_VERTEX_GRANULARITY", std::to_string(props.mesh_shader_ext.meshOutputPerVertexGranularity));
		defs.setDefinition("SHADER_MESH_PREFERS_COMPACT_PRIMITIVE_OUTPUT", tod(props.mesh_shader_ext.prefersCompactPrimitiveOutput));
		defs.setDefinition("SHADER_MESH_PREFERS_COMPACT_VERTEX_OUTPUT", tod(props.mesh_shader_ext.prefersCompactVertexOutput));
		defs.setDefinition("SHADER_MESH_PREFERS_LOCAL_INVOCATION_PRIMITIVE_OUTPUT", tod(props.mesh_shader_ext.prefersLocalInvocationPrimitiveOutput));
		defs.setDefinition("SHADER_MESH_PREFERS_LOCAL_INVOCATION_VERTEX_OUTPUT", tod(props.mesh_shader_ext.prefersLocalInvocationVertexOutput));

		defs.setDefinition("SHADER_FRAGMENT_BARYCENTRICS", tod(features.fragment_shader_barycentric_khr.fragmentShaderBarycentric));

		defs.setDefinition("SHADER_GEOMETRY_PROCESSING_STAGES_STORES_AND_ATOMICS", tod(features.features2.features.vertexPipelineStoresAndAtomics));
		defs.setDefinition("SHADER_FRAGMENT_STORES_AND_ATOMICS", tod(features.features2.features.fragmentStoresAndAtomics));

		defs.setDefinition("SHADER_BUFFER_INT64_ATOMICS", tod(features.features_12.shaderBufferInt64Atomics));
		defs.setDefinition("SHADER_SHARED_INT64_ATOMICS", tod(features.features_12.shaderSharedInt64Atomics));
		defs.setDefinition("SHADER_IMAGE_INT64_ATOMICS", tod(features.shader_image_atomic_int64_ext.shaderImageInt64Atomics));
		defs.setDefinition("SHADER_SPARSE_IMAGE_INT64_ATOMICS", tod(features.shader_image_atomic_int64_ext.sparseImageInt64Atomics));

		uint32_t atomic_float_16_flags = 0;
		uint32_t atomic_float_32_flags = 0;
		uint32_t atomic_float_64_flags = 0;
		if (features.shader_atomic_float_ext.shaderBufferFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if (features.shader_atomic_float_ext.shaderBufferFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if (features.shader_atomic_float_ext.shaderBufferFloat64Atomics)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if (features.shader_atomic_float_ext.shaderBufferFloat64AtomicAdd)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if (features.shader_atomic_float_ext.shaderSharedFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if (features.shader_atomic_float_ext.shaderSharedFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if (features.shader_atomic_float_ext.shaderSharedFloat64Atomics)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if (features.shader_atomic_float_ext.shaderSharedFloat64AtomicAdd)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if (features.shader_atomic_float_ext.shaderImageFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, IMAGE);
		if (features.shader_atomic_float_ext.shaderImageFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, IMAGE);
		if (features.shader_atomic_float_ext.sparseImageFloat32Atomics)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SPARSE_IMAGE);
		if (features.shader_atomic_float_ext.sparseImageFloat32AtomicAdd)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SPARSE_IMAGE);

		if (features.shader_atomic_float_2_ext.shaderBufferFloat16Atomics)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, BUFFER);
		if (features.shader_atomic_float_2_ext.shaderBufferFloat16AtomicAdd)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, BUFFER);
		if (features.shader_atomic_float_2_ext.shaderBufferFloat16AtomicMinMax)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if (features.shader_atomic_float_2_ext.shaderBufferFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if (features.shader_atomic_float_2_ext.shaderBufferFloat64AtomicMinMax)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, BUFFER);
		if (features.shader_atomic_float_2_ext.shaderSharedFloat16Atomics)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(XCHG, SHARED_MEMORY);
		if (features.shader_atomic_float_2_ext.shaderSharedFloat16AtomicAdd)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(ADD, SHARED_MEMORY);
		if (features.shader_atomic_float_2_ext.shaderSharedFloat16AtomicMinMax)
			atomic_float_16_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if (features.shader_atomic_float_2_ext.shaderSharedFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if (features.shader_atomic_float_2_ext.shaderSharedFloat64AtomicMinMax)
			atomic_float_64_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SHARED_MEMORY);
		if (features.shader_atomic_float_2_ext.shaderImageFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, IMAGE);
		if (features.shader_atomic_float_2_ext.sparseImageFloat32AtomicMinMax)
			atomic_float_32_flags |= SHADER_ATOMIC_FLOAT_BIT(MIN_MAX, SPARSE_IMAGE);

		defs.setDefinition("SHADER_ATOMIC_FLOAT_16_FLAGS", std::to_string(atomic_float_16_flags));
		defs.setDefinition("SHADER_ATOMIC_FLOAT_32_FLAGS", std::to_string(atomic_float_32_flags));
		defs.setDefinition("SHADER_ATOMIC_FLOAT_64_FLAGS", std::to_string(atomic_float_64_flags));

		defs.update();
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

	VkResult VkApplication::deviceWaitIdle()
	{
		// TODO aquire all queues mutexes
		VkResult res = vkDeviceWaitIdle(_device);
		return res;
	}

	void VkApplication::initSDL()
	{
		uint32_t init = 0;
		init |= SDL_INIT_VIDEO;
		init |= SDL_INIT_EVENTS;
		init |= SDL_INIT_GAMEPAD;
		if (!SDL_Init(init))
		{
			std::cout << SDL_GetError() << std::endl;
			exit(-1);
		}
		SDL_Vulkan_LoadLibrary(nullptr);
	}

	VkApplication::VkApplication(CreateInfo const& ci) :
		_name(ci.name)
	{
		const auto big_bang = std::chrono::system_clock::now();

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
			.gpu_id = static_cast<uint32_t>(ci.args.get<int>("--gpu")),
			.enable_validation = intToBool(ci.args.get<int>("--validation")),
			.enable_object_naming = intToBool(ci.args.get<int>("--name_vk_objects")),
			.enable_command_buffer_labels = intToBool(ci.args.get<int>("--cmd_labels")),
			.dump_shader_source = intToBool(ci.args.get<int>("--dump_shader_source")),
			.dump_shader_spv = intToBool(ci.args.get<int>("--dump_spv")),
		};

		std::string arg_image_layout = ci.args.get<std::string>("image_layout");
		_options.use_general_image_layout_bits = 0;
		if (arg_image_layout == "specific")
		{
			_options.use_general_image_layout_bits = 0;
		}
		else if (arg_image_layout == "general")
		{
			_options.use_general_image_layout_bits = uint64_t(-1);
		}
		else if (arg_image_layout == "auto")
		{
			// For now same as auto
			// TODO mask based on the physical device
		}
		else
		{	
			// TODO use a better parsing function (check error and parse hex)
			uint64_t mask = std::atoll(arg_image_layout.c_str());
			_options.use_general_image_layout_bits = mask;
		}

		bool mt = true;
		size_t n_threads = 0;
		_verbosity = ci.args.get<int>("--verbosity");

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

		_logger.max_verbosity = _verbosity;
		_logger.log_f = [this](std::string_view sv, Logger::Options options)
		{
			this->log(sv, options);
		};

		{
			const std::time_t big_bang_time_t = std::chrono::system_clock::to_time_t(big_bang);
			_logger("Engine started at "s + std::ctime(&big_bang_time_t), Logger::Options::TagSuccess);
		}

		_thread_pool = std::unique_ptr<DelayedTaskExecutor>(DelayedTaskExecutor::MakeNew(DelayedTaskExecutor::MakeInfo{
			.multi_thread = mt,
			.n_threads = n_threads,
			.logger = &_logger,
		}));
	}

	void VkApplication::init()
	{
		loadFileSystem();
		initSDL();
		preChecks();

		initInstance(_name);
		initValidLayers();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createCommandPools();

		fillCommonShaderDefinitions();

		_sampler_library = std::make_unique<SamplerLibrary>(SamplerLibrary::CI{
			.app = this,
			.name = "SamplerLibrary",
		});

		_texture_file_cache = std::make_unique<TextureFileCache>(TextureFileCache::CI{
			.app = this,
			.name = "TextureFileCache",
		});

		_prebuilt_transfer_commands = std::make_unique<PrebuilTransferCommands>(this);

		SlangResult slang_result = slang::createGlobalSession(_slang_session.writeRef());
		if (!SLANG_SUCCEEDED(slang_result))
		{
			_logger("Failed to init slang!", Logger::Options::TagError);
			_slang_session = nullptr;
		}
		else
		{
			
		}
		
	}

	void VkApplication::cleanup()
	{
		if (_slang_session)
		{
			slang::shutdown();
		}

		_prebuilt_transfer_commands.reset();
		_texture_file_cache.reset();
		_sampler_library.reset();

		_empty_set_layout = nullptr;

		_command_pools.clear();

		_queues_by_family.clear();
		_queues_by_desired.clear();

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

	VkApplication::~VkApplication()
	{
		cleanup();
	}

	void VkApplication::loadFileSystem()
	{
		const std::filesystem::path current_path = std::filesystem::current_path();
		const std::filesystem::path exe_path = that::GetCurrentExecutableAbsolutePath();
		const std::filesystem::path exe_folder = exe_path.parent_path();

		_file_system = std::make_unique<that::FileSystem>(that::FileSystem::CI{

		});

		that::MountingPoints & _mounting_points = _file_system->mountingPoints();
		using T = const char[4];
		// Default values
		_mounting_points["exec"] = exe_folder.string();
		_mounting_points["ShaderLib"] = exe_folder.string() + "/ShaderLib";
		
		_mounting_points["gen"] = exe_folder.string() + "/gen";

		std::filesystem::path mp_file_path = exe_folder / "MountingPoints.txt";

		if (std::filesystem::exists(mp_file_path))
		{
			std::string file;
			that::Result read_file_result = that::ReadFileToString(mp_file_path, file);
			if (read_file_result == that::Result::Success)
			{
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
						_mounting_points["ProjectShaders"] = _mounting_points["DevProjectsFolder"] / getProjectName() / "Shaders"s;
					}
				}
			
			}
			else
			{
				_logger("Could not find the Mounting Points file. Using Executable folder in place.", Logger::Options::TagHighWarning);
			};
		}
		if (!_mounting_points.contains("ProjectShaders"))
		{
			_mounting_points["ProjectShaders"] = exe_folder.string() + "/Shaders";
		}

		// Copy Mounting points to macros
		{
			that::FileSystem::MacroMap & macros = _file_system->macros();
			for (auto const& [key, value] : _mounting_points.getUnderlyingContainer())
			{
				macros[key] = value.native();
			}
		}

		// Fill default include directories
		{
			for (auto const& [key, value] : _mounting_points.getUnderlyingContainer())
			{
				_include_directories.push_back(value.parent_path());
			}
		}
	}

	void VkApplication::log(std::string_view sv, Logger::Options options)
	{
		const uint verbosity = static_cast<uint>(options & Logger::Options::_VerbosityMask);
		const bool can_log = verbosity <= _verbosity;
		if (can_log)
		{
			const bool lock_mutex = !(options & Logger::Options::NoLock);
			if (lock_mutex)
			{
				g_common_mutex.lock();
			}

			auto & stream = std::cout;
			if (!(options & Logger::Options::NoTime))
			{
				Clock::time_point rn = Clock::now();
				Clock::duration diff = (rn - _time_begin);
				LogDurationAsTimePoint(stream, diff);
			}

			const TagStr tag_str = GetTagStr(options);

			stream << tag_str.openning; 
			//stream << tag_str.token << " ";
			stream << sv;
			stream << tag_str.closing;
			if (!(options & Logger::Options::NoEndL))
			{
				stream << std::endl;
			}

			if (lock_mutex)
			{
				g_common_mutex.unlock();
			}
		}
	}
}
