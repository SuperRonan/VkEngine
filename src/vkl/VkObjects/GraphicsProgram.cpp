
#include <vkl/VkObjects/GraphicsProgram.hpp>

namespace vkl
{
	GraphicsProgramInstance::GraphicsProgramInstance(CreateInfo const& ci) :
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts.getInstance(),
			.shaders = ci.shaders,
		}),
		_vertex(ci.vertex),
		_tess_control(ci.tess_control),
		_tess_eval(ci.tess_eval),
		_geometry(ci.geometry),
		_task(ci.task),
		_mesh(ci.mesh),
		_fragment(ci.fragment)
	{
		createLayout();

		if (valid(_task) || valid(_mesh))
		{
			extractLocalSizeIFP();
		}
	}

	void GraphicsProgramInstance::extractLocalSizeIFP()
	{
		ShaderInstance* shader = valid(_task) ? task() : mesh();
		if (shader)
		{
			const auto& refl = shader->reflection();
			const auto& lcl = refl.entry_points[0].local_size;
			_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
		}
	}

	GraphicsProgram::GraphicsProgram(CreateInfoVertex const& civ) :
		Program(Program::CI{
			.app = civ.app,
			.name = civ.name,
			.sets_layouts = civ.sets_layouts,
			.hold_instance = civ.hold_instance,
		}),
		_vertex(addShader(civ.vertex)),
		_tess_control(addShader(civ.tess_control)),
		_tess_eval(addShader(civ.tess_eval)),
		_geometry(addShader(civ.geometry)),
		_fragment(addShader(civ.fragment))
	{
		setInvalidationCallbacks();
	}

	GraphicsProgram::GraphicsProgram(CreateInfoMesh const& cim) :
		Program(Program::CI{
			.app = cim.app,
			.name = cim.name,
			.sets_layouts = cim.sets_layouts,
			.hold_instance = cim.hold_instance,
		}),
		_task(addShader(cim.task)),
		_mesh(addShader(cim.mesh)),
		_fragment(addShader(cim.fragment))
	{
		setInvalidationCallbacks();
	}

	GraphicsProgram::~GraphicsProgram()
	{

	}

	void GraphicsProgram::createInstanceIFP()
	{
		MyVector<std::shared_ptr<ShaderInstance>> shaders(_shaders.size());
		for (size_t i = 0; i < shaders.size(); ++i)
		{
			shaders[i] = _shaders[i]->instance();
		}
		_inst = std::make_shared<GraphicsProgramInstance>(GraphicsProgramInstance::CI{
			.app = application(),
			.name = name(),
			.sets_layouts = _provided_sets_layouts,
			.shaders = std::move(shaders),
			.vertex = _vertex,
			.tess_control = _tess_control,
			.tess_eval = _tess_eval,
			.geometry = _geometry,
			.task = _task,
			.mesh = _mesh,
			.fragment = _fragment,
		});
	}
}