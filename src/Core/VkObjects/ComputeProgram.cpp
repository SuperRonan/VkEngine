
#include <Core/VkObjects/ComputeProgram.hpp>

namespace vkl
{
	ComputeProgramInstance::ComputeProgramInstance(CreateInfo const& ci) :
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts.getInstance(),
			.shaders = {ci.shader},
		})
	{
		createLayout();

		extractLocalSize();
	}

	void ComputeProgramInstance::extractLocalSize()
	{
		const auto& refl = shader()->reflection();
		const auto& lcl = refl.entry_points[0].local_size;
		assert(lcl.x != 0);
		assert(lcl.y != 0);
		assert(lcl.z != 0);
		_local_size = { .width = lcl.x, .height = lcl.y, .depth = lcl.z };
	}

	ComputeProgram::ComputeProgram(CreateInfo const& ci) :
		Program(Program::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
			.hold_instance = ci.hold_instance,
		})
	{
		_shaders = { ci.shader };

		setInvalidationCallbacks();
	}

	void ComputeProgram::createInstanceIFP()
	{
		_inst = std::make_shared<ComputeProgramInstance>(ComputeProgramInstance::CI{
			.app = application(),
			.name = name(),
			.sets_layouts = _provided_sets_layouts,
			.shader = shader()->instance(),
		});
	}

	ComputeProgram::~ComputeProgram()
	{

	}
}