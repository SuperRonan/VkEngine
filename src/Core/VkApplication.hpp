#pragma once

#include "VulkanCommons.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <Utils/stl_extension.hpp>
#include <memory>
#include <type_traits>

namespace vkl
{
	class StagingPool;
	class CommandPool;

	class VkApplication
	{
	public:
		
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics_family;
			std::optional<uint32_t> transfer_family;
			std::optional<uint32_t> present_family;
			std::optional<uint32_t> compute_family;

			bool isComplete()const;
		};

		struct Queues
		{
			VkQueue graphics, transfer, present, compute;
		};

		struct Pools
		{
			std::shared_ptr<CommandPool> graphics, transfer, compute;
		};

	protected:

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		VkInstance _instance;
		std::vector<VkExtensionProperties> _instance_extensions;
		VkDebugUtilsMessengerEXT _debug_messenger;
		VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
		VkSampleCountFlagBits _max_msaa;
		VkDevice _device;
		std::vector<VkExtensionProperties> _device_extensions;

		VmaAllocator _allocator;

		Queues _queues;

		QueueFamilyIndices _queue_family_indices;

		Pools _pools;

		std::unique_ptr<StagingPool> _staging_pool;

		PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT;

		virtual std::vector<const char*> getValidLayers();

		virtual std::vector<const char*> getDeviceExtensions(); 

		virtual std::vector<const char*> getInstanceExtensions();

		bool _enable_valid_layers = false;

		std::string _name = {};

		struct VulkanFeatures
		{
			VkPhysicalDeviceFeatures features = {};
			VkPhysicalDeviceVulkan11Features features_11 = {};
			VkPhysicalDeviceVulkan12Features features_12 = {};
			VkPhysicalDeviceVulkan13Features features_13 = {};
		};

		virtual void requestFeatures(VulkanFeatures & features);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

		bool checkValidLayerSupport();

		void preChecks();

		void initInstance(std::string const& name);

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)const;

		void initValidLayers();

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

		bool checkDeviceExtensionSupport(VkPhysicalDevice const& device);

		virtual bool isDeviceSuitable(VkPhysicalDevice const& device);

		virtual int64_t ratePhysicalDevice(VkPhysicalDevice const& device);

		VkSampleCountFlagBits getMaxUsableSampleCount();

		void pickPhysicalDevice();

		void createLogicalDevice();

		void createCommandPools();

		void createAllocator();

		void initGLFW();

		virtual void init();

		virtual void cleanup();


	public:

		VkApplication(std::string const& name, bool enable_validation_layers=false);

		VkApplication(VkApplication const&) = delete;

		VkApplication(VkApplication&&) = default;

		VkApplication& operator=(VkApplication const&) = delete;

		VkApplication& operator=(VkApplication&&) = default;

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

		StagingPool& stagingPool();

		void nameObject(VkDebugUtilsObjectNameInfoEXT const& object_to_name);
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

		constexpr VkObject(VkObject const& ) noexcept = default;
		constexpr VkObject(VkObject&&) noexcept = default;

		constexpr VkObject& operator=(VkObject const&) noexcept = default;
		constexpr VkObject& operator=(VkObject&&) noexcept = default;

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