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
		std::vector<std::shared_ptr<DescriptorSetLayout>> _set_layouts;
		std::vector<VkPushConstantRange> _push_constants;

		constexpr Program(VkApplication * app = nullptr) : 
			VkObject(app)
		{}

		Program(Program const& other) = delete;

		constexpr Program(Program && other) noexcept:
			VkObject(std::move(other)),
			_shaders(std::move(other._shaders)),
			_set_layouts(std::move(other._set_layouts)),
			_push_constants(std::move(other._push_constants))
		{}

		Program& operator=(Program const&) = delete;

		constexpr Program& operator=(Program&& other) noexcept
		{
			VkObject::operator=(std::move(other));

			_layout = std::move(other._layout);
			std::swap(_shaders, other._shaders);
			std::swap(_set_layouts, other._set_layouts);
			std::swap(_push_constants, other._push_constants);

			return *this;
		}

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

		constexpr PipelineLayout const& pipelineLayout()const
		{
			return _layout;
		}

		constexpr PipelineLayout & pipelineLayout()
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

	class GraphicsProgram : public Program 
	{
	public:

		struct CreateInfo 
		{
			// TODO all stages
			std::shared_ptr<Shader> _vertex = nullptr, _geometry = nullptr, _fragment = nullptr;

			VkApplication* getApplication()const;
		};

	protected:

		CreateInfo _ci;

	public:

		GraphicsProgram(CreateInfo const& ci);

	};

	class ComputeProgram : public Program
	{
	protected:

		VkExtent3D _local_size = { 0, 0, 0 };

	public:

		constexpr ComputeProgram(VkApplication * app = nullptr):
			Program(app)
		{}

		ComputeProgram(Shader&& shader);

		ComputeProgram(ComputeProgram const&) = delete;

		constexpr ComputeProgram(ComputeProgram && other) noexcept:
			Program(std::move(other)),
			_local_size(other._local_size)
		{}

		ComputeProgram& operator=(ComputeProgram const&) = delete;

		constexpr ComputeProgram& operator=(ComputeProgram&& other) noexcept
		{
			Program::operator=(std::move(other));
			std::swap(_local_size, other._local_size);
			return *this;
		}
		
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

	class RayTracingProgram : public Program
	{

	};
}