#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Execution/ResourcesToUpload.hpp>

#include <mutex>

namespace vkl
{
	struct TransferBudget
	{
		size_t bytes = 0;
		size_t instances = 0;
		
		constexpr TransferBudget& operator+=(TransferBudget const& o) noexcept
		{
			bytes += o.bytes;
			instances += o.instances;
			return *this;
		}

		constexpr TransferBudget& operator+=(size_t bytes) noexcept
		{
			this->bytes += bytes;
			++instances;
			return *this;
		}

		constexpr TransferBudget operator+(TransferBudget const& o) const noexcept
		{
			TransferBudget res = *this;
			res += o;
			return res;
		}

		constexpr TransferBudget operator+(size_t bytes) const noexcept
		{
			TransferBudget res = *this;
			res += bytes;
			return res;
		}

		constexpr bool withinLimit(TransferBudget const& limit) const
		{
			return (bytes < limit.bytes) && (instances < limit.instances);
		}
	};

	struct AsynchUpload
	{
		std::string name = {};
		std::vector<PositionedObjectView> sources = {};
		ObjectView source = {};
		
		std::shared_ptr<ImageView> target_view = nullptr;
		uint32_t buffer_row_length = 0;
		uint32_t buffer_image_height = 0;
		
		std::shared_ptr<Buffer> target_buffer = nullptr;

		CompletionCallback completion_callback = {};

		size_t getSize()const;
	};

	class UploadQueue : public VkObject
	{
	public:

	protected:

		std::mutex _mutex;
		std::deque<AsynchUpload> _queue;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		UploadQueue(CreateInfo const& ci);

		void enqueue(AsynchUpload const& upload);
		void enqueue(AsynchUpload && upload);

		using Budget = TransferBudget;

		ResourcesToUpload consume(Budget const& b);
	};

	struct AsynchMipsCompute
	{
		std::shared_ptr<ImageView> target = nullptr;
		CompletionCallback completion_callback = {};

		size_t getSize()const;
	};

	struct MipMapComputeQueue : public VkObject
	{
	public:

	protected:

		std::mutex _mutex;
		std::deque<AsynchMipsCompute> _queue;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		
		MipMapComputeQueue(CreateInfo const& ci);

		void enqueue(AsynchMipsCompute const& mc);
		void enqueue(AsynchMipsCompute && mc);

		
		using Budget = TransferBudget;

		std::vector<AsynchMipsCompute> consume(Budget const& b);
	};
}