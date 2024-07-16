#pragma once

#include "Shader.hpp"
#include "AbstractInstance.hpp"
#include "PipelineLayout.hpp"
#include <memory>

namespace vkl
{
	class ProgramInstance : public AbstractInstance
	{
	public:

		static constexpr uint32_t ShaderUnused()
		{
			return VK_SHADER_UNUSED_KHR;
		}
		
	protected:

		std::shared_ptr<PipelineLayoutInstance> _layout;
		MyVector<std::shared_ptr<ShaderInstance>> _shaders;
		MultiDescriptorSetsLayoutsInstances _provided_sets_layouts;
		MultiDescriptorSetsLayoutsInstances _reflection_sets_layouts;
		MultiDescriptorSetsLayoutsInstances _sets_layouts;
		MyVector<VkPushConstantRange> _push_constants;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayoutsInstances sets_layouts;
			MyVector<std::shared_ptr<ShaderInstance>> shaders;
		};
		using CI = CreateInfo;

		ProgramInstance(CreateInfo const& ci);

		virtual ~ProgramInstance() override;

	public:

		bool valid(uint32_t index) const
		{
			const bool res = index != ShaderUnused();
			assert(!res || index < _shaders.size32());
			return res;
		}

		ShaderInstance* getShaderSafe(uint32_t index)const
		{
			return valid(index) ? _shaders[index].get() : nullptr;
		}

		std::shared_ptr<ShaderInstance> getShaderSharedPtr(uint32_t index) const
		{
			return valid(index) ? _shaders[index] : nullptr;
		}

		bool reflect();

		bool checkSetsLayoutsMatch(std::ostream * stream = nullptr)const;

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
	public:

		static constexpr uint32_t ShaderUnused()
		{
			return VK_SHADER_UNUSED_KHR;
		}

	protected:

		using ParentType = InstanceHolder<ProgramInstance>;

		// Keep a pipeline layout descriptor here?
		
		MultiDescriptorSetsLayouts _provided_sets_layouts;

		MyVector<std::shared_ptr<Shader>> _shaders;

		std::shared_ptr<AsynchTask> _create_instance_task = nullptr;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		Program(CreateInfo const& ci);

		virtual ~Program()override;

		void setInvalidationCallbacks();

		void launchInstanceCreationTask();

		virtual void createInstanceIFP() = 0;

		virtual void destroyInstanceIFN() override;

	public:

		uint32_t addShader(std::shared_ptr<Shader> const& shader);

		bool valid(uint32_t index) const
		{
			const bool res = index != ShaderUnused();
			assert(!res || index < _shaders.size32());
			return res;
		}

		Shader* getShaderSafe(uint32_t index)const
		{
			return valid(index) ? _shaders[index].get() : nullptr;
		}

		std::shared_ptr<Shader> getShaderSharedPtr(uint32_t index) const
		{
			return valid(index) ? _shaders[index] : nullptr;
		}

		constexpr const auto& shaders()const
		{
			return _shaders;
		}

		constexpr auto& shaders()
		{
			return _shaders;
		}

		bool updateResources(UpdateContext & ctx);

		bool hasInstanceOrIsPending() const
		{
			return _inst || _create_instance_task;
		}

		void waitForInstanceCreationIFN();

		std::vector<std::shared_ptr<AsynchTask>> getShadersTasksDependencies()const;

		const std::shared_ptr<AsynchTask>& creationTask()const
		{
			return _create_instance_task;
		}
	};	
}