#pragma once

#include <Core/VulkanCommons.hpp>

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
	class StagingPool;
	class CommandPool;
	class DescriptorSetLayout;
	class DescriptorSetLayoutInstance;
	using DescriptorSetLayoutCache = GenericCache<DescriptorSetLayout>;
	template <class Key>
	using DescriptorSetLayoutCacheImpl = GenericCacheImpl<Key, DescriptorSetLayout>;

	class SamplerLibrary;



	template <class T>
	using SPtr = std::shared_ptr<T>;

	class VkApplication
	{
	public:

		struct Options
		{
			bool enable_validation = false;
			bool enable_object_naming = false;
			bool enable_command_buffer_labels = false;
		};

		static void FillArgs(argparse::ArgumentParser & args_parser);
		
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics_family = {};
			std::optional<uint32_t> transfer_family = {};
			std::optional<uint32_t> present_family = {};
			std::optional<uint32_t> compute_family = {};

			bool isComplete()const;
		};

		struct Queues
		{
			VkQueue graphics = VK_NULL_HANDLE, transfer = VK_NULL_HANDLE, present = VK_NULL_HANDLE, compute = VK_NULL_HANDLE;
		};

		struct Pools
		{
			SPtr<CommandPool> graphics = nullptr, transfer = nullptr, compute = nullptr;
		};

	protected:

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		Options _options = {};

		VkInstance _instance = VK_NULL_HANDLE;
		std::vector<VkExtensionProperties> _instance_extensions = {};
		VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
		VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
		VulkanDeviceProps _device_props = {};
		VkDevice _device = VK_NULL_HANDLE;
		std::vector<VkExtensionProperties> _device_extensions = {};

		VmaAllocator _allocator = nullptr;

		Queues _queues = {};

		QueueFamilyIndices _queue_family_indices = {};

		Pools _pools = {};


		

		struct ExtFunctionsPtr
		{
			PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = nullptr;
			
			PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;
			
			PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT = nullptr;
			PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT = nullptr;
			PFN_vkCmdInsertDebugUtilsLabelEXT _vkCmdInsertDebugUtilsLabelEXT = nullptr;
		};

		ExtFunctionsPtr _ext_functions;

		void loadExtFunctionsPtr();

		virtual std::vector<const char*> getValidLayers();

		virtual std::vector<const char*> getDeviceExtensions(); 

		virtual std::vector<const char*> getInstanceExtensions();

		std::string _name = {};

		VulkanFeatures _requested_features = {};
		VulkanFeatures _available_features = {};

		mutable std::shared_mutex _mutex;
		std::shared_ptr<DescriptorSetLayoutInstance> _empty_set_layout;
		std::vector<std::shared_ptr<DescriptorSetLayoutCache>> _desc_set_layout_caches;
		DescriptorSetBindingGlobalOptions _descriptor_binding_options;

		SamplerLibrary * _sampler_library = nullptr;

		std::unique_ptr<DelayedTaskExecutor> _thread_pool = nullptr;

		virtual void requestFeatures(VulkanFeatures & features);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

		bool checkValidLayerSupport();

		void preChecks();

		void initInstance(std::string const& name);

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)const;

		void initValidLayers();

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

		// Returns number of extensions 
		uint32_t checkDeviceExtensionSupport(VkPhysicalDevice const& device);

		virtual bool isDeviceSuitable(VkPhysicalDevice const& device);

		virtual int64_t ratePhysicalDevice(VkPhysicalDevice const& device);

		void pickPhysicalDevice();

		void createLogicalDevice();

		void createCommandPools();

		void createAllocator();

		void queryDescriptorBindingOptions();

		void initGLFW();

		virtual void init();

		virtual void cleanup();


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

		QueueFamilyIndices const& getQueueFamilyIndices()const;

		Queues const& queues()const;

		Pools const& pools()const;

		//StagingPool& stagingPool();

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

		static const constexpr uint32_t EXT_NONE = uint32_t(0);
		uint32_t getDeviceExtVersion(std::string_view ext_name) const;
		bool hasDeviceExtension(std::string_view ext_name) const
		{
			return getDeviceExtVersion(ext_name) != EXT_NONE;
		}
		uint32_t getInstanceExtVersion(std::string_view ext_name)const;
		bool hasInstanceExtension(std::string_view ext_name)const
		{
			return getInstanceExtVersion(ext_name) != EXT_NONE;
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
			assert(s < deviceProperties().props.limits.maxBoundDescriptorSets);
			return _desc_set_layout_caches[s];
		}

		template <class MakeCacheFunction>
		std::shared_ptr<DescriptorSetLayoutCache> getDescSetLayoutCacheOrEmplace(uint32_t s, MakeCacheFunction const& make_cache_function)
		{
			assert(s < deviceProperties().props.limits.maxBoundDescriptorSets);
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