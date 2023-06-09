#pragma once

#include "Command.hpp"
#include <string>
#include <unordered_map>
#include <Core/AbstractInstance.hpp>

namespace vkl
{
	using namespace std::chrono_literals;

	class DefinitionsMap
	{
	protected:

		using MapType = std::unordered_map<std::string, std::string>;
		MapType _definitions;
		
		std::vector<std::string> _collapsed;
		
		std::vector<InvalidationCallback> _invalidation_callbacks;
	
	public:

		DefinitionsMap() {};

		void setDefinition(std::string const& key, std::string const& value);

		const std::string & getDefinition(std::string const& key) const;

		void update();

		std::vector<std::string> const& collapsed()const
		{
			return _collapsed;
		}

		void callInvalidationCallbacks()
		{
			for (auto& ic : _invalidation_callbacks)
			{
				ic.callback();
			}
		}

		void addInvalidationCallback(InvalidationCallback const& ic)
		{
			_invalidation_callbacks.push_back(ic);
		}

		void removeInvalidationCallbacks(const VkObject* ptr)
		{
			for (auto it = _invalidation_callbacks.begin(); it < _invalidation_callbacks.end(); ++it)
			{
				if (it->id == ptr)
				{
					it = _invalidation_callbacks.erase(it);
				}
			}
		}
	};

	class Executor : public VkObject
	{
	protected:

		DefinitionsMap _common_definitions;

	public:

		template <class StringLike = std::string>
		Executor(VkApplication * app, StringLike && name):
			VkObject(app, std::forward<StringLike>(name))
		{}

		virtual void declare(std::shared_ptr<Command> cmd) = 0;

		virtual void declare(std::shared_ptr<ImageView> view) = 0;

		virtual void declare(std::shared_ptr<Buffer> buffer) = 0;

		void declare(std::shared_ptr<Mesh> mesh)
		{
			declare(mesh->combinedBuffer());
		}

		virtual void declare(std::shared_ptr<Sampler> sampler) = 0;

		virtual void init() = 0;

		virtual void updateResources() = 0;

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
	};
}