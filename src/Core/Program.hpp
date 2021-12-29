#pragma once

#include "Shader.hpp"
#include "PipelineLayout.hpp"
#include <memory>

namespace vkl
{
	class Program : public VkObject
	{
	protected:

		VkDescriptorSetLayout
		PipelineLayout _layout;
		std::vector<std::shared_ptr<Shader>> _shaders;



	public:

		void createLayout();

	};

	class GraphicsProgram : public Program
	{
	protected:

	public:

	};

	class ComputeProgram : public Program
	{
	protected:

	public:



	};
}