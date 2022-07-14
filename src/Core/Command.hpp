#pragma once

#include "VkApplication.hpp"
#include "CommandBuffer.hpp"
#include "Pipeline.hpp"
#include "DescriptorSet.hpp"
#include "Buffer.hpp"
#include "ImageView.hpp"
#include "Sampler.hpp"

#include <memory>
#include <vector>

namespace vkl
{
	class ExecutionContext
	{
	protected:

		std::vector<std::shared_ptr<CommandBuffer>> _command_buffers;

	public:

		void reset();

		void pushCommandBuffer(std::shared_ptr<CommandBuffer> cmd);

	};

	class ResourceBinding
	{
	public:
		
		enum class Type { 
			UNKNOWN = VK_DESCRIPTOR_TYPE_MAX_ENUM, 
			UBO = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			SSBO = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			IMAGE = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 
			TEXTURE = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 
			SAMPLER = VK_DESCRIPTOR_TYPE_SAMPLER, 
			SAMPLED_IMAGE = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		};

	protected:
		std::vector<std::shared_ptr<Buffer>> _buffers = {};
		std::vector<std::shared_ptr<ImageView>> _images = {};
		std::vector<std::shared_ptr<Sampler>> _samplers = {};
		uint32_t _binding = uint32_t(-1);
		uint32_t _set = 0;
		std::string _name = "";
		Type _type = Type::UNKNOWN;

	public:

		constexpr bool resolved()const
		{
			return _binding != uint32_t(-1);
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr Type type()const
		{
			return _type;
		}

		constexpr bool isBuffer()const
		{
			return _type == Type::UBO || _type == Type::SSBO;
		}

		constexpr bool isImage()const
		{
			return _type == Type::IMAGE || _type == Type::TEXTURE || _type == Type::SAMPLER || _type == Type::SAMPLED_IMAGE;
		}

		constexpr auto& buffers()
		{
			return _buffers;
		}

		constexpr auto& images()
		{
			return _images;
		}

		constexpr auto& samplers()
		{
			return _samplers;
		}

		constexpr VkDescriptorType vkType()const
		{
			return (VkDescriptorType) _type;
		}

		constexpr uint32_t set()const
		{
			return _set;
		}
		
		constexpr uint32_t binding()const
		{
			return _binding;
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

	class ShaderCommand : public Command
	{
	protected:

		std::shared_ptr<Pipeline> _pipeline;
		std::vector<std::shared_ptr<DescriptorPool>> _desc_pools;
		std::vector<std::shared_ptr<DescriptorSet>> _desc_sets;
		std::shared_ptr<CommandBuffer> _cmd;
		std::vector<ResourceBinding> _bindings;

	public:

		virtual ~ShaderCommand() = 0;

		virtual void createDescriptorSets();

		virtual void init() override = 0;

		virtual void execute(ExecutionContext & context) override = 0;

	};
}