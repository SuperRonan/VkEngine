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
			(prev._access != next._access || (accessIsReadAndWrite(prev._access) && accessIsReadAndWrite(next._access)))
			|| (is_image && layoutTransitionRequired(prev, next));
		return res;
	}

	class ExecutionContext
	{
	protected:

		std::shared_ptr<CommandBuffer> _current_command_buffer = nullptr;

		std::unordered_map<VkBuffer, ResourceState> _buffer_states;
		std::unordered_map<VkImageView, ResourceState> _image_states;

	public:

		void reset();

		ResourceState& getBufferState(VkBuffer b);

		ResourceState& getImageState(VkImageView i);

		constexpr std::shared_ptr<CommandBuffer>& getCurrentCommandBuffer()
		{
			return _current_command_buffer;
		}


	};

	struct Resource
	{
		std::vector<std::shared_ptr<Buffer>> _buffers = {};
		std::vector<std::shared_ptr<ImageView>> _images = {};
		ResourceState _beging_state = {};
		std::optional<ResourceState> _end_state = {}; // None means the same as begin state
		VkImageUsageFlags _image_usage = 0;
		VkBufferUsageFlags _buffer_usage = 0;

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
}