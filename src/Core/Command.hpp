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

		size_t _frame_event_counter = 0;

		std::vector<std::shared_ptr<CommandBuffer>> _command_buffers_to_submit;

		std::unordered_map<VkBuffer, ResourceState> _buffer_states;
		std::unordered_map<VkImageView, ResourceState> _image_states;

	public:

		void reset();

		ResourceState& getBufferState(VkBuffer b);

		ResourceState& getImageState(VkImageView i);

		std::shared_ptr<CommandBuffer> getCurrentCommandBuffer();
		

	};

	struct Resource
	{
		std::vector<std::shared_ptr<Buffer>> _buffers = {};
		std::vector<std::shared_ptr<ImageView>> _images = {};
		ResourceState _state = {};
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		constexpr bool isImage() const
		{
			return !_images.empty();
		}

		constexpr bool isBuffer() const
		{
			return !_buffers.empty();
		}
	};

	class ResourceBinding
	{
	public:
		
	protected:
		Resource _resource;
		std::vector<std::shared_ptr<Sampler>> _samplers = {};
		uint32_t _binding = uint32_t(-1);
		uint32_t _set = 0;
		std::string _name = "";
		VkDescriptorType _type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	public:

		constexpr bool resolved()const
		{
			return _binding != uint32_t(-1);
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr auto type()const
		{
			return _type;
		}

		constexpr bool isBuffer()const
		{
			return 
				_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || 
				_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		constexpr bool isImage()const
		{
			return 
				_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || 
				_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || 
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr bool isSampler()
		{
			return 
				_type == VK_DESCRIPTOR_TYPE_SAMPLER || 
				_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		constexpr auto& buffers()
		{
			return _resource._buffers;
		}

		constexpr auto& images()
		{
			return _resource._images;
		}

		constexpr auto& samplers()
		{
			return _samplers;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType) _type;
		}

		constexpr auto set()const
		{
			return _set;
		}
		
		constexpr auto binding()const
		{
			return _binding;
		}

		constexpr const auto & state()const
		{
			return _resource._state;
		}
		constexpr auto& state()
		{
			return _resource._state;
		}


		constexpr const auto& resource()const
		{
			return _resource;
		}

		constexpr auto& resource()
		{
			return _resource;
		}
	};

	class Command : public VkObject
	{
	protected:

	public:

		virtual ~Command() = 0;

		virtual void init() = 0;

		virtual void execute(ExecutionContext & context) = 0;

	};

	class DeviceCommand : public Command
	{
	protected:
		
		std::vector<Resource> _resources;

	public:

		virtual void recordInputSynchronization(CommandBuffer & cmd, ExecutionContext & context);

	};

	class ShaderCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Pipeline> _pipeline;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
		
		std::vector<ResourceBinding> _bindings;

		std::vector<uint8_t> _push_constants_data;
		

	public:

		virtual ~ShaderCommand() = 0;

		virtual void extractBindingsFromReflection();

		virtual void writeDescriptorSets();

		virtual void declareDescriptorSetsResources();

		virtual void recordBindings(CommandBuffer & cmd, ExecutionContext & context);

		virtual void recordCommandBuffer(CommandBuffer & cmd, ExecutionContext & context) = 0;

		virtual void init() override = 0;

		virtual void execute(ExecutionContext & context) override = 0;

	};

	class ComputeCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<ComputeProgram> _program;
		VkExtent3D _dispatch_size;
		bool _dispatch_threads = false;

	public:

		virtual void recordCommandBuffer(CommandBuffer & cmd, ExecutionContext & context) override;

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;
	};

	class GraphicsCommand : public ShaderCommand
	{
	protected:

		std::shared_ptr<RenderPass> _render_pass;
		std::shared_ptr<Framebuffer> _framebuffer;
		
		// Here ?
		std::shared_ptr<Buffer> _mesh_buffer;

	public:

		virtual void init() override;

		virtual void declareGraphicsResources();

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) = 0;

		virtual void recordCommandBuffer(CommandBuffer & cmd, ExecutionContext & context) override;

		virtual void execute(ExecutionContext& context) override;
	};

	class DrawMeshCommand : public GraphicsCommand
	{
	protected:

		std::vector<std::shared_ptr<Mesh>> _meshes;

	public:

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;
	};

	class FragCommand : public GraphicsCommand
	{
	protected:

	public:

		virtual void init() override;

		virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context) override;

	};

	class BlitImage : public DeviceCommand
	{
	protected:

		std::shared_ptr<ImageView> _src, _dst;
		std::vector<VkImageBlit> _regions;
		VkFilter _filter = VK_FILTER_NEAREST;

	public:

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

	};
}