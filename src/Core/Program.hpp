#pragma once

#include "Shader.hpp"
#include "PipelineLayout.hpp"
#include <memory>

namespace vkl
{
	class Program : public VkObject
	{
	protected:
		 
		PipelineLayout _layout;
		std::vector<std::shared_ptr<Shader>> _shaders;
		std::vector<DescriptorSetLayout> _set_layouts;
		std::vector<VkPushConstantRange> _push_constants;

		constexpr Program(VkApplication * app = nullptr) : 
			VkObject(app)
		{}

	public:
		    
		bool buildSetLayouts();

		bool buildPushConstantRanges();

		void createLayout();

		constexpr const auto& shaders()const
		{
			return _shaders;
		}

		constexpr auto& shaders()
		{
			return _shaders;
		}

		constexpr PipelineLayout const& pipelineLayout()const
		{
			return _layout;
		}

		constexpr PipelineLayout & pipelineLayout()
		{
			return _layout;
		}

	};

	class GraphicsProgram : public Program 
	{
	protected:

	public:

	};

	class ComputeProgram : public Program
	{
	protected:

		VkExtent3D _local_size = { 0, 0, 0 };

	public:

		ComputeProgram(Shader&& shader);
		
		void extractLocalSize();

		constexpr const VkExtent3D& localSize()const
		{
			return _local_size;
		}

		constexpr const std::shared_ptr<Shader>& shader()const
		{
			return _shaders.front();
		}

		constexpr std::shared_ptr<Shader>& shader()
		{
			return _shaders.front();
		}
	};
}