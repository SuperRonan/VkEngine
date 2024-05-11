#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/App/VkApplication.hpp>

namespace vkl
{
	class Queue : public VkObject
	{
	protected:

		VkDeviceQueueCreateFlags _flags = 0;
		uint32_t _family_index = 0;
		uint32_t _index = 0;
		VkQueue _handle = VK_NULL_HANDLE;

		VkQueueFamilyProperties _properties;
		float _priority = std::numeric_limits<float>::quiet_NaN();

		std::mutex _mutex;

		void get();

		void setVkNameIFP();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			VkDeviceQueueCreateFlags flags = 0;
			uint32_t family_index = 0;
			uint32_t index = 0;
			VkQueueFamilyProperties properties;
			float priority = std::numeric_limits<float>::quiet_NaN(); // NaN == unknown priority
		};
		using CI = CreateInfo;

		Queue(CreateInfo const& ci);

		virtual ~Queue() override;

		constexpr VkQueue handle()const
		{
			return _handle;
		}

		constexpr operator VkQueue()const
		{
			return handle();
		}

		constexpr VkDeviceQueueCreateFlags flags()const
		{
			return _flags;
		}

		constexpr uint32_t familyIndex()const
		{
			return _family_index;
		}

		constexpr uint32_t index() const
		{
			return _index;
		}

		constexpr const VkQueueFamilyProperties& properties()const
		{
			return _properties;
		}

		constexpr float priority()const
		{
			return _priority;
		}

		std::mutex const& mutex()const
		{
			return _mutex;
		}

		std::mutex & mutex()
		{
			return _mutex;
		}
	};
}