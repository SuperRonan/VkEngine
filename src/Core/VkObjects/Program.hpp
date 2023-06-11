#pragma once

#include "Shader.hpp"
#include "AbstractInstance.hpp"
#include "PipelineLayout.hpp"
#include <memory>

namespace vkl
{
	class ProgramInstance : public VkObject
	{
	protected:

		std::shared_ptr<PipelineLayout> _layout;
		std::vector<std::shared_ptr<ShaderInstance>> _shaders;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _set_layouts;
		std::vector<VkPushConstantRange> _push_constants;

		ProgramInstance(VkApplication* app, std::string const& name) :
			VkObject(app, name)
		{}

		virtual ~ProgramInstance() override {}

	public:

		bool reflect();

		void createLayout();

		constexpr const auto& shaders()const
		{
			return _shaders;
		}

		constexpr auto& shaders()
		{
			return _shaders;
		}

		constexpr const auto & pipelineLayout()const
		{
			return _layout;
		}

		constexpr const auto& setLayouts() const
		{
			return _set_layouts;
		}

		constexpr auto& setLayouts()
		{
			return _set_layouts;
		}

		constexpr const auto& pushConstantRanges()const
		{
			return _push_constants;
		}

		constexpr auto& pushConstantRanges()
		{
			return _push_constants;
		}

	};

	class Program : public InstanceHolder<ProgramInstance>
	{
	protected:

		using ParentType = InstanceHolder<ProgramInstance>;
		
		std::vector<std::shared_ptr<Shader>> _shaders;

		Program(VkApplication* app, std::string const& name):
			ParentType(app, name)
		{}

		virtual ~Program()override
		{
			destroyInstance();
			for (auto& shader : _shaders)
			{
				shader->removeInvalidationCallbacks(this);
			}
		}

		virtual void createInstance() = 0;

		void destroyInstance();

	public:

		constexpr const auto& shaders()const
		{
			return _shaders;
		}

		constexpr auto& shaders()
		{
			return _shaders;
		}

		bool updateResources(UpdateContext & ctx);
	};

	class GraphicsProgramInstance : public ProgramInstance
	{
	protected:

		std::shared_ptr<ShaderInstance> _vertex = nullptr;
		std::shared_ptr<ShaderInstance> _geometry = nullptr;
		std::shared_ptr<ShaderInstance> _fragment = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ShaderInstance> vertex = nullptr;
			std::shared_ptr<ShaderInstance> geometry = nullptr;
			std::shared_ptr<ShaderInstance> fragment = nullptr;
		};
		using CI = CreateInfo;

		GraphicsProgramInstance(CreateInfo const& ci);

		virtual ~GraphicsProgramInstance() override {};

	};

	class GraphicsProgram : public Program 
	{
	public:

		struct CreateInfo 
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Shader> vertex = nullptr; 
			std::shared_ptr<Shader> geometry = nullptr;
			std::shared_ptr<Shader> fragment = nullptr;
		};
		using CI = CreateInfo;

	protected:

		std::shared_ptr<Shader> _vertex = nullptr;
		std::shared_ptr<Shader> _geometry = nullptr;
		std::shared_ptr<Shader> _fragment = nullptr;

		virtual void createInstance() override;

	public:

		GraphicsProgram(CreateInfo const& ci);

		virtual ~GraphicsProgram() override;

	};

	class ComputeProgramInstance : public ProgramInstance
	{
	protected:

		std::shared_ptr<ShaderInstance> _shader = nullptr;
		VkExtent3D _local_size = makeZeroExtent3D();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ShaderInstance> shader = nullptr;
		};
		using CI = CreateInfo;

		ComputeProgramInstance(CreateInfo const& ci);

		virtual ~ComputeProgramInstance() override {};

		void extractLocalSize();

		constexpr const VkExtent3D& localSize()const
		{
			return _local_size;
		}

		constexpr const std::shared_ptr<ShaderInstance>& shader()const
		{
			return _shader;
		}

		constexpr std::shared_ptr<ShaderInstance>& shader()
		{
			return _shader;
		}

	};

	class ComputeProgram : public Program
	{
	protected:

		std::shared_ptr<Shader> _shader = nullptr;

		virtual void createInstance() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Shader> shader = nullptr;
		};
		using CI = CreateInfo;

		ComputeProgram(CreateInfo const& ci);

		virtual ~ComputeProgram() override;


		constexpr const std::shared_ptr<Shader>& shader()const
		{
			return _shader;
		}

		constexpr std::shared_ptr<Shader>& shader()
		{
			return _shader;
		}
	};
}