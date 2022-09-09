#pragma once

#include "ShaderCommand.hpp"

namespace vkl
{
	class ComputeCommand : public ShaderCommand
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path shader_path;
			VkExtent3D dispatch_size = makeZeroExtent3D();
			bool dispatch_threads = false;
			std::vector<ShaderBindingDescriptor> bindings = {};
			std::vector<std::string> definitions = {};
		};
		using CI = CreateInfo;

	protected:

		std::shared_ptr<ComputeProgram> _program = nullptr;
		VkExtent3D _dispatch_size = makeZeroExtent3D();
		bool _dispatch_threads = false;

	public:

		ComputeCommand(CreateInfo const& ci);

		virtual ~ComputeCommand() override = default;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

		constexpr void setDispatchSize(VkExtent3D size)
		{
			_dispatch_size = size;
		}

		constexpr void setDispatchType(bool type_is_threads)
		{
			_dispatch_threads = type_is_threads;
		}
		
		constexpr VkExtent3D getDispatchSize()const
		{
			return _dispatch_size;
		}

		constexpr VkExtent3D getWorkgroupsDispatchSize()const
		{
			const VkExtent3D res = _dispatch_threads ? VkExtent3D{
				.width = std::moduloCeil(_dispatch_size.width, _program->localSize().width),
				.height = std::moduloCeil(_dispatch_size.height, _program->localSize().height),
				.depth = std::moduloCeil(_dispatch_size.depth, _program->localSize().depth),
			} : _dispatch_size;
			return res;
		}

		constexpr VkExtent3D getThreadsDispatchSize()const
		{
			const VkExtent3D res = _dispatch_threads ? _dispatch_size : VkExtent3D{
				.width = (_dispatch_size.width * _program->localSize().width),
				.height = (_dispatch_size.height * _program->localSize().height),
				.depth = (_dispatch_size.depth * _program->localSize().depth),
			};
			return res;
		}
	};
}