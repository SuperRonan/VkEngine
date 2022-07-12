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
		
		enum class Type { NONE, UBO, SSBO, IMAGE, TEXTURE, SAMPLER, SAMPLED_IMAGE };

	protected:
		std::vector<std::shared_ptr<Buffer>> _buffers = {};
		std::vector<std::shared_ptr<ImageView>> _images = {};
		std::vector<std::shared_ptr<Sampler>> _sampler = {};
		uint32_t _binding = uint32_t(-1);
		uint32_t _set = 0;
		std::string _name = "";
		Type _type = Type::NONE;

	public:

		constexpr bool resolved()const
		{
			return _binding != uint32_t(-1);
		}

		const std::string& name()const
		{
			return _name;
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