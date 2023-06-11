#pragma once

#include <Core/App/VkApplication.hpp>

namespace vkl
{
	class Semaphore : public VkObject
	{
	protected:

		VkSemaphore _handle = VK_NULL_HANDLE;

	public:

		constexpr Semaphore() noexcept = default;

		constexpr Semaphore(VkApplication* app, VkSemaphore handle) noexcept :
			VkObject(app),
			_handle(handle)
		{}

		Semaphore(VkApplication* app);

		template <typename StringLike = std::string>
		Semaphore(VkApplication* app, StringLike&& name) :
			VkObject(app, std::forward<StringLike>(name))
		{
			create();
		}

		virtual ~Semaphore() override;

		void create();

		void destroy();

		constexpr operator VkSemaphore() const
		{
			return _handle;
		}

		constexpr VkSemaphore handle()const
		{
			return _handle;
		}
	};
}