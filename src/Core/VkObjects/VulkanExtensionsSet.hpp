#pragma once

#include <Core/VulkanCommons.hpp>


namespace vkl
{
	class VulkanExtensionsSet
	{
	public:
		static const constexpr uint32_t EXT_NONE = uint32_t(0);
	protected:

		VkPhysicalDevice _device = VK_NULL_HANDLE;
		const char * _p_layer_name = nullptr;
		MyVector<VkExtensionProperties> _queried_props = {};
		std::unordered_map<std::string_view, uint32_t> _extensions = {};
		MyVector<const char *> _p_exts = {};

		void recordExtensions(std::function<bool(std::string_view)> const& filter = {});

		void findExtensions();

	public:

		VulkanExtensionsSet(VkPhysicalDevice device = VK_NULL_HANDLE, const char * p_layer_name = nullptr);

		//template <::std::concepts::StringLike Str, ::std::concepts::Set<Str> S>
		//VulkanExtensionsSet(S const& desired_extensions, VkPhysicalDevice device = VK_NULL_HANDLE, const char* p_layer_name = nullptr) :
		//	_device(device),
		//	_p_layer_name(p_layer_name)
		//{
		//	findExtensions();
		//	recordExtensions([&](std::string_view ext_name) {
		//		return desired_extensions.contains(ext_name);
		//	});
		//}

		VulkanExtensionsSet(std::set<std::string_view> const& desired_extensions, VkPhysicalDevice device = VK_NULL_HANDLE, const char* p_layer_name = nullptr);

		uint32_t getVersion(std::string_view ext_name) const;

		bool contains(std::string_view ext_name) const
		{
			return getVersion(ext_name) != EXT_NONE;
		}

		const MyVector<const char*>& pExts()const
		{
			return _p_exts;
		}
	};

	using VkExtSet = VulkanExtensionsSet;
}