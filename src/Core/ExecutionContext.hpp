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
    struct ResourceState
	{
		VkAccessFlags _access = VK_ACCESS_NONE_KHR;
		VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags _stage = VK_PIPELINE_STAGE_NONE_KHR;
	};

	constexpr bool accessIsWrite(VkAccessFlags access)
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

	constexpr bool accessIsRead(VkAccessFlags access)
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

	constexpr bool accessIsReadAndWrite(VkAccessFlags access)
	{
		return accessIsRead(access) && accessIsWrite(access);
	}

	constexpr bool layoutTransitionRequired(ResourceState prev, ResourceState next)
	{
		return (prev._layout != next._layout);
	}

	constexpr bool stateTransitionRequiresSynchronization(ResourceState prev, ResourceState next, bool is_image)
	{
		const bool res =
			!(accessIsRead(prev._access) && accessIsRead(next._access)) // Assuming that !read = write
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	struct ResourceStateTracker
	{
		std::unordered_map<VkBuffer, ResourceState> _buffer_states;
		std::unordered_map<ImageRange, ResourceState> _image_states;
	};

	class ExecutionContext
	{
	protected:

		std::shared_ptr<CommandBuffer> _command_buffer = nullptr;

		ResourceStateTracker * _reosurce_states;


	public:

		ExecutionContext(ResourceStateTracker * rst, std::shared_ptr<CommandBuffer> cmd);

		ResourceState& getBufferState(std::shared_ptr<Buffer> b);

		ResourceState& getImageState(std::shared_ptr<ImageView> i);

		constexpr std::shared_ptr<CommandBuffer>& getCommandBuffer()
		{
			return _command_buffer;
		}

		void setBufferState(std::shared_ptr<Buffer> b, ResourceState const& s);

		void setImageState(std::shared_ptr<ImageView> v, ResourceState const& s);

		void setCommandBuffer(std::shared_ptr<CommandBuffer> cmd);

	};

	struct Resource
	{
		std::vector<std::shared_ptr<Buffer>> _buffers = {};
		std::vector<std::shared_ptr<ImageView>> _images = {};
		ResourceState _begin_state = {};
		std::optional<ResourceState> _end_state = {}; // None means the same as begin state
		VkImageUsageFlags _image_usage = 0;
		VkBufferUsageFlags _buffer_usage = 0;

		// Can't have a constructor and still an aggregate initialization :'(
		//Resource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);

		constexpr bool isImage() const
		{
			return !_images.empty();
		}

		constexpr bool isBuffer() const
		{
			return !_buffers.empty();
		}

		const std::string& name()const;
	};

	Resource MakeResource(std::shared_ptr<Buffer> buffer, std::shared_ptr<ImageView> image);
}

