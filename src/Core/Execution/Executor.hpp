#pragma once

#include <Core/Commands/Command.hpp>
#include <string>

#include <Core/Execution/DefinitionMap.hpp>
#include <Core/Commands/ShaderCommand.hpp>

namespace vkl
{
	using namespace std::chrono_literals;

	class Executor : public VkObject
	{
	protected:

		DefinitionsMap _common_definitions;

		ShaderBindings _common_bindings;

	public:

		template <class StringLike = std::string>
		Executor(VkApplication * app, StringLike && name):
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual void declare(std::shared_ptr<Command> cmd) = 0;

		virtual void declare(std::shared_ptr<ImageView> view) = 0;

		virtual void declare(std::shared_ptr<Buffer> buffer) = 0;

		virtual void release(std::shared_ptr<Buffer> buffer) = 0;

		virtual void declare(std::shared_ptr<Mesh> mesh) = 0;

		virtual void declare(std::shared_ptr<Sampler> sampler) = 0;

		virtual void init() = 0;

		virtual void updateResources() = 0;

		virtual void execute(Command& cmd) = 0;

		virtual void execute(std::shared_ptr<Command> cmd) = 0;

		virtual void execute(Executable const& executable) = 0;

		void operator()(std::shared_ptr<Command> cmd)
		{
			execute(cmd);
		}

		void operator()(Executable const& executable)
		{
			execute(executable);
		}

		virtual void waitForAllCompletion(uint64_t timeout = UINT64_MAX) = 0;

		DefinitionsMap& getCommonDefinitions()
		{
			return _common_definitions;
		}

		const DefinitionsMap& getCommonDefinitions()const
		{
			return _common_definitions;
		}

		ShaderBindings const& getCommonBindings() const
		{
			return _common_bindings;
		}

		ShaderBindings& getCommonBindings()
		{
			return _common_bindings;
		}
	};
}