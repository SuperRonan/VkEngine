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
			DynamicValue<VkExtent3D> dispatch_size = {};
			bool dispatch_threads = false;
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::string> definitions = {};
		};
		using CI = CreateInfo;

	protected:

		std::filesystem::path _shader_path;
		std::vector<std::string> _definitions;

		std::shared_ptr<ComputeProgram> _program = nullptr;
		DynamicValue<VkExtent3D> _dispatch_size = {};
		bool _dispatch_threads = false;

	public:

		ComputeCommand(CreateInfo const& ci);

		virtual ~ComputeCommand() override = default;

		virtual void recordCommandBuffer(CommandBuffer& cmd, ExecutionContext& context) override;

		virtual void init() override;

		virtual void execute(ExecutionContext& context) override;

		virtual bool updateResources()override;

		void setDispatchSize(DynamicValue<VkExtent3D> size)
		{
			_dispatch_size = size;
		}

		constexpr void setDispatchType(bool type_is_threads)
		{
			_dispatch_threads = type_is_threads;
		}
		
		constexpr const DynamicValue<VkExtent3D> & getDispatchSize()const
		{
			return _dispatch_size;
		}

		constexpr VkExtent3D getWorkgroupsDispatchSize()const
		{
			const VkExtent3D res = _dispatch_threads ? VkExtent3D{
				.width = std::divCeil(_dispatch_size.value().width, _program->localSize().width),
				.height = std::divCeil(_dispatch_size.value().height, _program->localSize().height),
				.depth = std::divCeil(_dispatch_size.value().depth, _program->localSize().depth),
			} : _dispatch_size.value();
			return res;
		}

		constexpr VkExtent3D getThreadsDispatchSize()const
		{
			const VkExtent3D res = _dispatch_threads ? _dispatch_size.value() : VkExtent3D{
				.width = (_dispatch_size.value().width * _program->localSize().width),
				.height = (_dispatch_size.value().height * _program->localSize().height),
				.depth = (_dispatch_size.value().depth * _program->localSize().depth),
			};
			return res;
		}
	};
}