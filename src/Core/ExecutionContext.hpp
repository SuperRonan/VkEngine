#pragma once

#include "VkApplication.hpp"
#include "CommandBuffer.hpp"
#include "Pipeline.hpp"
#include "DescriptorSet.hpp"
#include "Buffer.hpp"
#include "ImageView.hpp"
#include "Sampler.hpp"
#include "RenderPass.hpp"
#include "Framebuffer.hpp"
#include "Mesh.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace vkl
{
	struct ImageRange
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageSubresourceRange range = {};

		constexpr bool operator==(ImageRange const& other)const
		{
			using namespace vk_operators;
			return (image == other.image) && (range == other.range);
		}
	};
}

namespace std
{
	template<>
	struct hash<vkl::ImageRange>
	{
		size_t operator()(vkl::ImageRange const& ir) const
		{
			size_t addr = reinterpret_cast<size_t>(ir.image);
			return addr;
		}
	};
}

namespace vkl
{
	struct ResourceState1
	{
		VkAccessFlags _access = VK_ACCESS_NONE_KHR;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags _stage = VK_PIPELINE_STAGE_NONE_KHR;
	};
	
    struct ResourceState2
	{
		VkAccessFlags2 _access = VK_ACCESS_2_NONE_KHR;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags2 _stage = VK_PIPELINE_STAGE_2_NONE_KHR;
	};

	constexpr bool accessIsWrite1(VkAccessFlags access)
	{
		return access & (
			VK_ACCESS_HOST_WRITE_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_SHADER_WRITE_BIT |
			VK_ACCESS_TRANSFER_WRITE_BIT |
			VK_ACCESS_MEMORY_WRITE_BIT
			);
	}

	constexpr bool accessIsWrite2(VkAccessFlags2 access)
	{
		return access & (
			VK_ACCESS_2_HOST_WRITE_BIT |
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_2_SHADER_WRITE_BIT |
			VK_ACCESS_2_TRANSFER_WRITE_BIT |
			VK_ACCESS_2_MEMORY_WRITE_BIT
			);
	}

	constexpr bool accessIsRead1(VkAccessFlags access)
	{
		return access & (
			VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
			VK_ACCESS_INDEX_READ_BIT |
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
			VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
			VK_ACCESS_SHADER_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_TRANSFER_READ_BIT |
			VK_ACCESS_HOST_READ_BIT |
			VK_ACCESS_MEMORY_READ_BIT
		);
	}

	constexpr bool accessIsRead2(VkAccessFlags2 access)
	{
		return access & (
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT |
			VK_ACCESS_2_INDEX_READ_BIT |
			VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT |
			VK_ACCESS_2_UNIFORM_READ_BIT |
			VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_SHADER_READ_BIT |
			VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_TRANSFER_READ_BIT |
			VK_ACCESS_2_HOST_READ_BIT |
			VK_ACCESS_2_MEMORY_READ_BIT
		);
	}

	constexpr bool accessIsReadAndWrite1(VkAccessFlags access)
	{
		return accessIsRead1(access) && accessIsWrite1(access);
	}

	constexpr bool accessIsReadAndWrite2(VkAccessFlags2 access)
	{
		return accessIsRead1(access) && accessIsWrite1(access);
	}

	constexpr bool layoutTransitionRequired(ResourceState1 prev, ResourceState1 next)
	{
		return (prev._layout != next._layout);
	}

	constexpr bool layoutTransitionRequired(ResourceState2 prev, ResourceState2 next)
	{
		return (prev._layout != next._layout);
	}

	constexpr bool stateTransitionRequiresSynchronization1(ResourceState1 prev, ResourceState1 next, bool is_image)
	{
		const bool res =
			!(accessIsRead1(prev._access) && accessIsRead1(next._access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	constexpr bool stateTransitionRequiresSynchronization2(ResourceState2 prev, ResourceState2 next, bool is_image)
	{
		const bool res =
			!(accessIsRead2(prev._access) && accessIsRead2(next._access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	struct ResourceStateTracker
	{
		std::unordered_map<VkBuffer, ResourceState2> _buffer_states;
		std::unordered_map<ImageRange, ResourceState2> _image_states;
	};

	class ExecutionContext
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		ResourceStateTracker * _reosurce_states;

		std::vector<std::shared_ptr<VkObject>> _objects_to_keep;


	public:

		ExecutionContext(ResourceStateTracker * rst, std::shared_ptr<CommandBuffer> cmd);

		ResourceState2& getBufferState(std::shared_ptr<Buffer> b);

		ResourceState2& getImageState(std::shared_ptr<ImageView> i);

		constexpr std::shared_ptr<CommandBuffer>& getCommandBuffer()
		{
			return _command_buffer;
		}

		void setBufferState(std::shared_ptr<Buffer> b, ResourceState2 const& s);

		void setImageState(std::shared_ptr<ImageView> v, ResourceState2 const& s);

		void setCommandBuffer(std::shared_ptr<CommandBuffer> cmd);

		void keppAlive(std::shared_ptr<VkObject> obj);
	};

	struct Resource
	{
		std::shared_ptr<Buffer> _buffer = {};
		std::shared_ptr<ImageView> _image = {};
		ResourceState2 _begin_state = {};
		std::optional<ResourceState2> _end_state = {}; // None means the same as begin state
		VkImageUsageFlags _image_usage = 0;
		VkBufferUsageFlags _buffer_usage = 0;

		// Can't have a constructor and still an aggregate initialization :'(
		//Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

		bool isImage() const
		{
			return !!_image;
		}

		bool isBuffer() const
		{
			return !!_buffer;
		}

		const std::string& name()const;
	};

	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

	//class Executable
	//{
	//public:
	//	using ExecutableFunction = std::function<void(ExecutionContext& ctx)>;
	//protected:

	//	ExecutableFunction _function;

	//public:

	//	Executable(ExecutableFunction const& f):
	//		_function(f)
	//	{}

	//	void operator()(ExecutionContext& ctx)
	//	{
	//		_function(ctx);
	//	}
	//};
	using Executable = std::function<void(ExecutionContext& ctx)>;
}

