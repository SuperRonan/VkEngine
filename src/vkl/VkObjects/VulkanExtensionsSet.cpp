#include <vkl/VkObjects/VulkanExtensionsSet.hpp>

namespace vkl
{

	VulkanExtensionsSet::VulkanExtensionsSet(VkPhysicalDevice device, const char* p_layer_name) :
		_device(device),
		_p_layer_name(p_layer_name)
	{
		findExtensions();
		recordExtensions();
	}

	VulkanExtensionsSet::VulkanExtensionsSet(std::set<std::string_view> const& desired_extensions, VkPhysicalDevice device, const char* p_layer_name) :
		_device(device),
		_p_layer_name(p_layer_name)
	{
		findExtensions();
		recordExtensions([&](std::string_view ext_name) {
			return desired_extensions.contains(ext_name);
		});
	}

	void VulkanExtensionsSet::recordExtensions(std::function<bool(std::string_view)> const& filter)
	{
		_p_exts.clear();
		_p_exts.reserve(_queried_props.size());
		for (size_t i = 0; i < _queried_props.size(); ++i)
		{
			std::string_view sv = _queried_props[i].extensionName;
			if (!filter || filter(sv))
			{
				_extensions[sv] = _queried_props[i].specVersion;
				_p_exts.push_back(sv.data());
			}
		}
	}

	void VulkanExtensionsSet::findExtensions()
	{
		_queried_props.clear();
		uint32_t count = 0;
		if (_device)
		{
			vkEnumerateDeviceExtensionProperties(_device, _p_layer_name, &count, nullptr);
			_queried_props.resize(count);
			vkEnumerateDeviceExtensionProperties(_device, _p_layer_name, &count, _queried_props.data());
		}
		else
		{
			vkEnumerateInstanceExtensionProperties(_p_layer_name, &count, nullptr);
			_queried_props.resize(count);
			vkEnumerateInstanceExtensionProperties(_p_layer_name, &count, _queried_props.data());
		}
	}

	uint32_t VulkanExtensionsSet::getVersion(std::string_view ext_name) const
	{
		uint32_t res = EXT_NONE;
		if (_extensions.contains(ext_name))
		{
			res = _extensions.at(ext_name);
		}
		return res;
	}
}