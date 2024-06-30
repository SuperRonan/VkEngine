#pragma once

#include <Core/VkObjects/Program.hpp>

namespace vkl
{
	class ComputeProgramInstance : public ProgramInstance
	{
	protected:

		VkExtent3D _local_size = makeUniformExtent3D(0);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
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
			assert(_shaders.size() == 1);
			return _shaders[0];
		}

		constexpr std::shared_ptr<ShaderInstance>& shader()
		{
			assert(_shaders.size() == 1);
			return _shaders[0];
		}

	};

	class ComputeProgram : public Program
	{
	protected:

		virtual void createInstanceIFP() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<Shader> shader = nullptr;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		ComputeProgram(CreateInfo const& ci);

		virtual ~ComputeProgram() override;


		constexpr const std::shared_ptr<Shader>& shader()const
		{
			assert(_shaders.size() == 1);
			return _shaders[0];
		}

		constexpr std::shared_ptr<Shader>& shader()
		{
			assert(_shaders.size() == 1);
			return _shaders[0];
		}
	};
}