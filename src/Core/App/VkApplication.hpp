#pragma once

#include <Core/VulkanCommons.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <Core/Utils/stl_extension.hpp>
#include <memory>
#include <type_traits>
#include <Core/Commands/ShaderBindingDescriptor.hpp>

namespace vkl
{
	class StagingPool;
	class CommandPool;

	template <class T>
	using SPtr = std::shared_ptr<T>;

	class VkApplication
	{
	public:
		
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

		//std::unique_ptr<StagingPool> _staging_pool;

		PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;

		struct ExtFunctionsPtr
		{
			PFN_vkCmdDrawMeshTasksEXT _vkCmdDrawMeshTasksEXT = nullptr;
		};

		ExtFunctionsPtr _ext_functions;

		void loadExtFunctionsPtr();

		virtual std::vector<const char*> getValidLayers();

		virtual std::vector<const char*> getDeviceExtensions(); 

		virtual std::vector<const char*> getInstanceExtensions();

		bool _enable_valid_layers = false;

		std::string _name = {};

		VulkanFeatures _requested_features = {};
		VulkanFeatures _available_features = {};

		DescriptorSetBindingGlobalOptions _descriptor_binding_options;

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

		VkApplication(std::string const& name, bool enable_validation_layers=false);

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

		void nameObject(VkDebugUtilsObjectNameInfoEXT const& object_to_name);

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

		constexpr bool enableValidation()const
		{
			return _enable_valid_layers;
		}

		const ExtFunctionsPtr& extFunctions()
		{
			return _ext_functions;
		}

		const DescriptorSetBindingGlobalOptions& descriptorBindingGlobalOptions() const
		{
			return _descriptor_binding_options;
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