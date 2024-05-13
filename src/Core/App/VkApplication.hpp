#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/VkObjects/VulkanExtensionsSet.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <memory>
#include <type_traits>
#include <shared_mutex>

#include <Core/Utils/stl_extension.hpp>

#include <Core/VkObjects/GenericCache.hpp>

#include <Core/Execution/ThreadPool.hpp>


namespace argparse
{
	class ArgumentParser;
}

namespace vkl
{
	// This is provided by the project
	// It works only if VkEngine is statically linked with the project exec
	extern const char * GetProjectName();
	
	class Queue;
	class CommandPool;
	class DescriptorSetLayout;
	class DescriptorSetLayoutInstance;
	using DescriptorSetLayoutCache = GenericCache<DescriptorSetLayout>;
	template <class Key>
	using DescriptorSetLayoutCacheImpl = GenericCacheImpl<Key, DescriptorSetLayout>;

	class SamplerLibrary;
	class TextureFileCache;


	template <class T>
	using SPtr = std::shared_ptr<T>;

	struct PrebuilTransferCommands;

	class VkApplication
	{
	public:

		struct Options
		{
			bool enable_validation = false;
			bool enable_object_naming = false;
			bool enable_command_buffer_labels = false;
			uint32_t gpu_id = uint32_t(-1);
			// bit field per image usage (VkImageUsageFlagBits)
			uint64_t use_general_image_layout_bits = 0;

			constexpr VkImageLayout getLayout(VkImageLayout optimal_layout, VkImageUsageFlags usage) const
			{
				VkImageLayout res = optimal_layout;
				if (use_general_image_layout_bits & usage)
				{
					res = VK_IMAGE_LAYOUT_GENERAL;
				}
				return res;
			}
		};

		static void FillArgs(argparse::ArgumentParser & args_parser);
		
		struct DesiredQueueInfo
		{
			VkQueueFlags flags = 0;
			float priority = 0;
			bool required = false;
			std::string name = {};

			//constexpr bool operator==(DesiredQueueInfo const& other) const noexcept
			//{
			//	return (flags == other.flags) && (priority == other.priority) && (required == other.required);
			//}

			//constexpr size_t hash()const noexcept
			//{
			//	std::hash<VkQueueFlags> hi;
			//	std::hash<bool> hb;
			//	std::hash<float> hf;
			//	std::hash<size_t> hs;
			//	return hs(hi(flags) ^ hf(priority) ^ hb(required));
			//}
			//static_assert(std::concepts::HashableFromMethod<DesiredQueueInfo>);
		};

		struct DesiredQueuesInfo
		{
			MyVector<DesiredQueueInfo> queues = {};
			bool need_presentation = false;

			VkQueueFlags totalFlags()const;
		};

		struct QueueIndex
		{
			uint32_t family = 0;
			uint32_t index = 0;
		};

	protected:

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		Options _options = {};

		VkInstance _instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
		VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
		VulkanDeviceProps _device_props = {};
		VkDevice _device = VK_NULL_HANDLE;

		VmaAllocator _allocator = nullptr;

		MyVector<MyVector<std::shared_ptr<Queue>>> _queues_by_family = {};
		MyVector<std::shared_ptr<CommandPool>> _command_pools = {};
		// Might be sparse (some elements might be nullptr)
		MyVector<QueueIndex> _queues_by_desired = {};


		std::unique_ptr<VulkanExtensionsSet> _instance_extensions;
		std::unique_ptr<VulkanExtensionsSet> _device_extensions;


		struct ExtFunctionsPtr
		{
			PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = nullptr;
			PFN_vkCmdDrawMeshTasksIndirectEXT _vkCmdDrawMeshTasksIndirectEXT = nullptr;
			PFN_vkCmdDrawMeshTasksIndirectCountEXT _vkCmdDrawMeshTasksIndirectCountEXT = nullptr;
			
			PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;
			
			PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT = nullptr;
			PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT = nullptr;
			PFN_vkCmdInsertDebugUtilsLabelEXT _vkCmdInsertDebugUtilsLabelEXT = nullptr;

			PFN_vkCreateAccelerationStructureKHR _vkCreateAccelerationStructureKHR = nullptr;
			PFN_vkDestroyAccelerationStructureKHR _vkDestroyAccelerationStructureKHR = nullptr;
			PFN_vkCmdBuildAccelerationStructuresKHR _vkCmdBuildAccelerationStructuresKHR = nullptr;
			PFN_vkCmdBuildAccelerationStructuresIndirectKHR _vkCmdBuildAccelerationStructuresIndirectKHR = nullptr;
			PFN_vkBuildAccelerationStructuresKHR _vkBuildAccelerationStructuresKHR = nullptr;
			PFN_vkCopyAccelerationStructureKHR _vkCopyAccelerationStructureKHR = nullptr;
			PFN_vkCopyAccelerationStructureToMemoryKHR _vkCopyAccelerationStructureToMemoryKHR = nullptr;
			PFN_vkCopyMemoryToAccelerationStructureKHR _vkCopyMemoryToAccelerationStructureKHR = nullptr;
			PFN_vkWriteAccelerationStructuresPropertiesKHR _vkWriteAccelerationStructuresPropertiesKHR = nullptr;
			PFN_vkCmdCopyAccelerationStructureKHR _vkCmdCopyAccelerationStructureKHR = nullptr;
			PFN_vkCmdCopyAccelerationStructureToMemoryKHR _vkCmdCopyAccelerationStructureToMemoryKHR = nullptr;
			PFN_vkCmdCopyMemoryToAccelerationStructureKHR _vkCmdCopyMemoryToAccelerationStructureKHR = nullptr;
			PFN_vkGetAccelerationStructureDeviceAddressKHR _vkGetAccelerationStructureDeviceAddressKHR = nullptr;
			PFN_vkCmdWriteAccelerationStructuresPropertiesKHR _vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
			PFN_vkGetDeviceAccelerationStructureCompatibilityKHR _vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
			PFN_vkGetAccelerationStructureBuildSizesKHR _vkGetAccelerationStructureBuildSizesKHR = nullptr;

		};

		ExtFunctionsPtr _ext_functions;

		void loadExtFunctionsPtr();

		virtual std::set<std::string_view> getValidLayers();

		virtual std::set<std::string_view> getDeviceExtensions();

		virtual std::set<std::string_view> getInstanceExtensions();
		
		virtual DesiredQueuesInfo getDesiredQueuesInfo();

		std::string _name = {};

		VulkanFeatures _available_features = {};

		mutable std::shared_mutex _mutex;
		std::shared_ptr<DescriptorSetLayoutInstance> _empty_set_layout;
		std::vector<std::shared_ptr<DescriptorSetLayoutCache>> _desc_set_layout_caches;
		DescriptorSetBindingGlobalOptions _descriptor_binding_options;

		std::unique_ptr<SamplerLibrary> _sampler_library = nullptr;

		std::unique_ptr<DelayedTaskExecutor> _thread_pool = nullptr;

		std::unique_ptr<TextureFileCache> _texture_file_cache = nullptr;

		std::unique_ptr<PrebuilTransferCommands> _prebuilt_transfer_commands = nullptr;

		MountingPoints _mounting_points;

		virtual void requestFeatures(VulkanFeatures & features);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

		bool checkValidLayerSupport();

		void preChecks();

		void initInstance(std::string const& name);

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)const;

		void initValidLayers();

		struct DeviceCandidateQueues
		{
			struct QueueInfo
			{
				VkQueueFamilyProperties props;
				MyVector<float> priorities;
				MyVector<std::string> names;
			};
			// Indexed by queue family id
			MyVector<QueueInfo> queues;
			MyVector<uint32_t> present_queues;
			MyVector<QueueIndex> desired_to_info;
			VkQueueFlags total_flags = 0;
			bool all_required = true;
		};

		struct DesiredDeviceInfo
		{
			std::set<std::string_view> extensions;
			VulkanFeatures features;
			DesiredQueuesInfo queues;
		};

		struct CandidatePhysicalDevice
		{
			VkPhysicalDevice device;
			
			VulkanExtensionsSet extensions;
			VulkanFeatures features;
			DeviceCandidateQueues queues;
			VulkanDeviceProps props;
		};

		DeviceCandidateQueues findQueueFamilies(VkPhysicalDevice device, DesiredQueuesInfo const& desired_queues);

		virtual bool isDeviceSuitable(CandidatePhysicalDevice const& candiate, DesiredDeviceInfo const& desired);

		virtual int64_t ratePhysicalDevice(VkPhysicalDevice device, DesiredDeviceInfo const& desired);

		// fills:
		// - _physical_device
		// - _device_extensions
		// - _device_props
		// 
		void pickPhysicalDevice();

		void createLogicalDevice();

		void createCommandPools();

		void createAllocator();

		void queryDescriptorBindingOptions();

		void initSDL();

		virtual void init();

		virtual void cleanup();

		void loadMountingPoints();

	public:

		struct CreateInfo
		{
			std::string name = {};
			argparse::ArgumentParser& args;
		};
		using CI = CreateInfo;

		VkApplication(CreateInfo const& ci);

		virtual ~VkApplication();

		virtual void run() = 0;

		constexpr VkInstance instance()const
		{
			return _instance;
		}

		constexpr VkPhysicalDevice physicalDevice()const
		{
			return _physical_device;
		}

		constexpr VkDevice device()const
		{
			return _device;
		}

		constexpr VmaAllocator allocator()const
		{
			return _allocator;
		}


		const auto& queuesByFamily()const
		{
			return _queues_by_family;
		}

		const auto& desiredQueuesIndices()const
		{
			return _queues_by_desired;
		}

		const auto& commandPools()const
		{
			return _command_pools;
		}

		void nameVkObjectIFP(VkDebugUtilsObjectNameInfoEXT const& object_to_name);

		void nameVkObjectIFP(VkObjectType type, uint64_t handle, std::string_view const& name);

		constexpr const VulkanFeatures& availableFeatures() const
		{
			return _available_features;
		}

		const VulkanDeviceProps& deviceProperties()const
		{
			return _device_props;
		}

		
		const VulkanExtensionsSet& instanceExtensions() const
		{
			return *_instance_extensions;
		}

		const VulkanExtensionsSet& deviceExtensions() const
		{
			return *_device_extensions;
		}

		const ExtFunctionsPtr& extFunctions()
		{
			return _ext_functions;
		}

		const DescriptorSetBindingGlobalOptions& descriptorBindingGlobalOptions() const
		{
			return _descriptor_binding_options;
		}

		std::shared_ptr<DescriptorSetLayoutCache> & getDescSetLayoutCache(uint32_t s)
		{
			assert(s < deviceProperties().props2.properties.limits.maxBoundDescriptorSets);
			return _desc_set_layout_caches[s];
		}

		template <class MakeCacheFunction>
		std::shared_ptr<DescriptorSetLayoutCache> getDescSetLayoutCacheOrEmplace(uint32_t s, MakeCacheFunction const& make_cache_function)
		{
			assert(s < deviceProperties().props2.properties.limits.maxBoundDescriptorSets);
			std::unique_lock lock(_mutex);
			std::shared_ptr<DescriptorSetLayoutCache> & res = _desc_set_layout_caches[s];
			if (!res)
			{
				res = make_cache_function();
			}
			return res;
		}

		SamplerLibrary& getSamplerLibrary()
		{
			return *_sampler_library;
		}

		DelayedTaskExecutor& threadPool()
		{
			return *_thread_pool;
		}

		const Options& options()const
		{
			return _options;
		}

		std::shared_ptr<DescriptorSetLayoutInstance> getEmptyDescSetLayout();

		TextureFileCache& textureFileCache()
		{
			return *_texture_file_cache;
		}

		PrebuilTransferCommands& getPrebuiltTransferCommands()
		{
			assert(_prebuilt_transfer_commands);
			return *_prebuilt_transfer_commands;
		}

		MountingPoints& mountingPoints()
		{
			return _mounting_points;
		}

		MountingPoints const & mountingPoints()const
		{
			return _mounting_points;
		}
	};

	class VkObject
	{
	protected:

		VkApplication* _app = nullptr;
		std::string _name = {};

	public:

		constexpr VkObject() noexcept = default;

		constexpr VkObject(VkApplication* app) noexcept :
			_app(app)
		{}

		template <typename StringLike = std::string>
		constexpr VkObject(VkApplication* app, StringLike && name) noexcept :
			_app(app),
			_name(std::forward<StringLike>(name))
		{}

		VkObject(VkObject const& ) noexcept = delete;
		VkObject(VkObject&&) noexcept = delete;

		VkObject& operator=(VkObject const&) noexcept = delete;
		VkObject& operator=(VkObject&&) noexcept = delete;

		virtual ~VkObject() {};

		constexpr VkApplication* application() noexcept
		{
			return _app;
		}

		constexpr const VkApplication* application() const noexcept
		{
			return _app;
		}

		constexpr void setApplication(VkApplication* app) noexcept
		{
			_app = app;
		}

		constexpr VkDevice device()const
		{
			return _app->device();
		}

		constexpr std::string const& name()const
		{
			return _name;
		}

		template <typename StringLike = std::string>
		constexpr void setName(StringLike && new_name)
		{
			_name = std::forward<StringLike>(new_name);
		}
	};
}