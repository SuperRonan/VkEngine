#pragma once

#include "Shader.hpp"
#include "AbstractInstance.hpp"
#include "PipelineLayout.hpp"
#include <memory>

namespace vkl
{
	class ProgramInstance : public AbstractInstance
	{
	protected:

		std::shared_ptr<PipelineLayout> _layout;
		std::vector<std::shared_ptr<ShaderInstance>> _shaders;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _provided_sets_layouts;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _reflection_sets_layouts;
		std::vector<std::shared_ptr<DescriptorSetLayout>> _sets_layouts;
		std::vector<VkPushConstantRange> _push_constants;

		ProgramInstance(VkApplication* app, std::string const& name);

		virtual ~ProgramInstance() override;

	public:

		bool reflect();

		bool checkSetsLayoutsMatch()const;

		void createLayout();

		constexpr const auto& shaders()const
		{
			return _shaders;
		}

		constexpr const auto & pipelineLayout()const
		{
			return _layout;
		}

		constexpr const auto& providedSetsLayouts()const
		{
			return _provided_sets_layouts;
		}

		constexpr const auto& reflectionSetsLayouts() const
		{
			return _reflection_sets_layouts;
		}

		constexpr const auto& setsLayouts() const
		{
			return _sets_layouts;
		}

		constexpr const auto& pushConstantRanges()const
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

		virtual ~Program()override;

		void addShadersInvalidationCallbacks();

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
		std::shared_ptr<ShaderInstance> _tess_control = nullptr;
		std::shared_ptr<ShaderInstance> _tess_eval = nullptr;
		std::shared_ptr<ShaderInstance> _geometry = nullptr;
		
		std::shared_ptr<ShaderInstance> _task = nullptr;
		std::shared_ptr<ShaderInstance> _mesh = nullptr;

		std::shared_ptr<ShaderInstance> _fragment = nullptr;

		// Mesh (or task if present) local_size
		VkExtent3D _local_size = makeZeroExtent3D();

		void extractLocalSize();

	public:

		struct CreateInfoVertex
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ShaderInstance> vertex = nullptr;
			std::shared_ptr<ShaderInstance> tess_control = nullptr;
			std::shared_ptr<ShaderInstance> tess_eval = nullptr;
			std::shared_ptr<ShaderInstance> geometry = nullptr;
			std::shared_ptr<ShaderInstance> fragment = nullptr;
		};
		using CIV = CreateInfoVertex;

		struct CreateInfoMesh
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ShaderInstance> task = nullptr;
			std::shared_ptr<ShaderInstance> mesh = nullptr;
			std::shared_ptr<ShaderInstance> fragment = nullptr;
		};
		using CIM = CreateInfoMesh;

		GraphicsProgramInstance(CreateInfoVertex const& civ);
		GraphicsProgramInstance(CreateInfoMesh const& cim);

		virtual ~GraphicsProgramInstance() override {};

		constexpr const VkExtent3D& localSize()const
		{
			return _local_size;
		}

	};

	class GraphicsProgram : public Program 
	{
	public:

		struct CreateInfoVertex
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Shader> vertex = nullptr; 
			std::shared_ptr<Shader> tess_control = nullptr;
			std::shared_ptr<Shader> tess_eval = nullptr;
			std::shared_ptr<Shader> geometry = nullptr;
			std::shared_ptr<Shader> fragment = nullptr;
		};
		using CIV = CreateInfoVertex;

		struct CreateInfoMesh
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Shader> task = nullptr;
			std::shared_ptr<Shader> mesh = nullptr;
			std::shared_ptr<Shader> fragment = nullptr;
		};

	protected:

		std::shared_ptr<Shader> _vertex = nullptr;
		std::shared_ptr<Shader> _tess_control = nullptr;
		std::shared_ptr<Shader> _tess_eval = nullptr;
		std::shared_ptr<Shader> _geometry = nullptr;
		
		std::shared_ptr<Shader> _task = nullptr;
		std::shared_ptr<Shader> _mesh = nullptr;

		std::shared_ptr<Shader> _fragment = nullptr;

		virtual void createInstance() override;

	public:

		GraphicsProgram(CreateInfoVertex const& ci);
		GraphicsProgram(CreateInfoMesh const& ci);

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