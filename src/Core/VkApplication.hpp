#pragma once

#include "VulkanCommons.hpp"
#include "StagingPool.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <Utils/stl_extension.hpp>

namespace vkl
{
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
			VkCommandPool graphics, transfer, compute;
		};

	protected:

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debug_messenger;
		VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
		VkSampleCountFlagBits _max_msaa;
		VkDevice _device;

		VmaAllocator _allocator;

		Queues _queues;

		QueueFamilyIndices _queue_family_indices;

		Pools _pools;

		StagingPool _staging_pool;

		virtual std::vector<const char*> getValidLayers()const;

		virtual std::vector<const char* > getDeviceExtensions()const; 

		std::vector<const char*> getRequiredExtensions();

		bool _enable_valid_layers = false;

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

		virtual void init(std::string const& name);

		virtual void cleanup();

	public:

		VkApplication(std::string const& name, bool enable_validation_layers=false);

		VkApplication(VkApplication const&) = delete;

		VkApplication(VkApplication&&) = default;

		VkApplication& operator=(VkApplication const&) = delete;

		VkApplication& operator=(VkApplication&&) = default;

		virtual ~VkApplication();

		virtual void run();

		VkInstance instance()const;

		VkPhysicalDevice physicalDevice()const;

		VkDevice device()const;

		VmaAllocator allocator()const;

		QueueFamilyIndices const& getQueueFamilyIndices()const;

		Queues const& queues()const;

		Pools const& pools()const;

		StagingPool& stagingPool();

		VkCommandBuffer beginSingleTimeCommand(VkCommandPool pool);

		void endSingleTimeCommandAndWait(VkCommandBuffer command_buffer, VkQueue submit_queue, VkCommandPool pool);
	};

	class VkObject
	{
	protected:

		VkApplication* _app = nullptr;

	public:

		constexpr VkObject() noexcept = default;

		constexpr VkObject(VkApplication* app) noexcept:
			_app(app)
		{}
		constexpr VkObject(VkObject const& ) noexcept = default;
		constexpr VkObject(VkObject&&) noexcept = default;

		constexpr VkObject& operator=(VkObject const&) noexcept = default;
		constexpr VkObject& operator=(VkObject&&) noexcept = default;

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
	};
}