#pragma once

#include <Core/VkObjects/Program.hpp>

namespace vkl
{
	class GraphicsProgramInstance : public ProgramInstance
	{
	protected:

		uint32_t _vertex = ShaderUnused();
		uint32_t _tess_control = ShaderUnused();
		uint32_t _tess_eval = ShaderUnused();
		uint32_t _geometry = ShaderUnused();
		uint32_t _task = ShaderUnused();
		uint32_t _mesh = ShaderUnused();
		uint32_t _fragment = ShaderUnused();

		// Mesh (or task if present) local_size
		VkExtent3D _local_size = makeZeroExtent3D();

		void extractLocalSizeIFP();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			MyVector<std::shared_ptr<ShaderInstance>> shaders = {};
			uint32_t vertex = ShaderUnused();
			uint32_t tess_control = ShaderUnused();
			uint32_t tess_eval = ShaderUnused();
			uint32_t geometry = ShaderUnused();
			uint32_t task = ShaderUnused();
			uint32_t mesh = ShaderUnused();
			uint32_t fragment = ShaderUnused();
		};
		using CI = CreateInfo;

		GraphicsProgramInstance(CreateInfo const& ci);

		virtual ~GraphicsProgramInstance() override = default;

		constexpr const VkExtent3D& localSize()const
		{
			return _local_size;
		}


		uint32_t vertexIndex()const
		{
			return _vertex;
		}

		uint32_t tessControlIndex()const
		{
			return _tess_control;
		}

		uint32_t tassEvalIndex()const
		{
			return _tess_eval;
		}

		uint32_t geometryIndex()const
		{
			return _geometry;
		}

		uint32_t taskIndex()const
		{
			return _task;
		}

		uint32_t meshIndex()const
		{
			return _mesh;
		}

		uint32_t fragmentIndex()const
		{
			return _fragment;
		}


		ShaderInstance* vertex()const
		{
			return getShaderSafe(_vertex);
		}

		ShaderInstance* tessControl()const
		{
			return getShaderSafe(_tess_control);
		}

		ShaderInstance* tessEval()const
		{
			return getShaderSafe(_tess_eval);
		}

		ShaderInstance* geometry()const
		{
			return getShaderSafe(_geometry);
		}

		ShaderInstance* task()const
		{
			return getShaderSafe(_task);
		}

		ShaderInstance* mesh()const
		{
			return getShaderSafe(_mesh);
		}

		ShaderInstance* fragment()const
		{
			return getShaderSafe(_fragment);
		}
	};

	class GraphicsProgram : public Program
	{
	public:

		struct CreateInfoVertex
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<Shader> vertex = nullptr;
			std::shared_ptr<Shader> tess_control = nullptr;
			std::shared_ptr<Shader> tess_eval = nullptr;
			std::shared_ptr<Shader> geometry = nullptr;
			std::shared_ptr<Shader> fragment = nullptr;
			Dyn<bool> hold_instance = true;
		};
		using CIV = CreateInfoVertex;

		struct CreateInfoMesh
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<Shader> task = nullptr;
			std::shared_ptr<Shader> mesh = nullptr;
			std::shared_ptr<Shader> fragment = nullptr;
			Dyn<bool> hold_instance = true;
		};

	protected:

		uint32_t _vertex = ShaderUnused();
		uint32_t _tess_control = ShaderUnused();
		uint32_t _tess_eval = ShaderUnused();
		uint32_t _geometry = ShaderUnused();
		uint32_t _task = ShaderUnused();
		uint32_t _mesh = ShaderUnused();
		uint32_t _fragment = ShaderUnused();

		virtual void createInstanceIFP() override;

	public:

		GraphicsProgram(CreateInfoVertex const& ci);
		GraphicsProgram(CreateInfoMesh const& ci);

		virtual ~GraphicsProgram() override;


		uint32_t vertexIndex()const
		{
			return _vertex;
		}

		uint32_t tessControlIndex()const
		{
			return _tess_control;
		}

		uint32_t tassEvalIndex()const
		{
			return _tess_eval;
		}
		
		uint32_t geometryIndex()const
		{
			return _geometry;
		}

		uint32_t taskIndex()const
		{
			return _task;
		}

		uint32_t meshIndex()const
		{
			return _mesh;
		}

		uint32_t fragmentIndex()const
		{
			return _fragment;
		}

		
		Shader* vertex()const
		{
			return getShaderSafe(_vertex);
		}

		Shader* tessControl()const
		{
			return getShaderSafe(_tess_control);
		}

		Shader* tessEval()const
		{
			return getShaderSafe(_tess_eval);
		}

		Shader* geometry()const
		{
			return getShaderSafe(_geometry);
		}

		Shader* task()const
		{
			return getShaderSafe(_task);
		}

		Shader* mesh()const
		{
			return getShaderSafe(_mesh);
		}

		Shader* fragment()const
		{
			return getShaderSafe(_fragment);
		}
	};
}