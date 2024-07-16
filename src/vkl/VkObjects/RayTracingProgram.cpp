
#include <vkl/VkObjects/RayTracingProgram.hpp>

namespace vkl
{
	RayTracingProgramInstance::RayTracingProgramInstance(CreateInfo const& ci) :
		ProgramInstance(ProgramInstance::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts.getInstance()
		}),
		_group_begin(ci.group_begin)
	{
		_shaders = ci.shaders;
		_shader_groups = ci.shader_groups;

		SpvReflectShaderModule const& refl = _shaders[0]->reflection();

		createLayout();
	}





	RayTracingProgram::RayTracingProgram(CreateInfo const& ci) :
		Program(Program::CI{
			.app = ci.app,
			.name = ci.name,
			.sets_layouts = ci.sets_layouts,
			.hold_instance = ci.hold_instance,
			})
	{
		const auto check_shader = [](std::shared_ptr<Shader> const& shader, VkShaderStageFlagBits stage)
		{
			assert(!!shader);
			assert(shader->stage() == stage);
		};
		const auto check_opt_shader = [](std::shared_ptr<Shader> const& shader, VkShaderStageFlagBits stage)
		{
			if (!!shader)
			{
				assert(shader->stage() == stage);
			}
		};

		_shader_groups.reserve(1 + ci.misses.size() + ci.hit_groups.size() + ci.callables.size());

		check_shader(ci.raygen, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		const uint32_t raygen_index = addShader(ci.raygen);
		_group_begin[static_cast<uint32_t>(ShaderRecordType::RayGen)] = _shader_groups.size32();
		_shader_groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.pNext = nullptr,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = raygen_index,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
			.pShaderGroupCaptureReplayHandle = nullptr,
		});

		_group_begin[static_cast<uint32_t>(ShaderRecordType::Miss)] = _shader_groups.size32();
		for (size_t m = 0; m < ci.misses.size(); ++m)
		{
			std::shared_ptr<Shader> const& miss_shader = ci.misses[m];
			check_shader(miss_shader, VK_SHADER_STAGE_MISS_BIT_KHR);
			const uint32_t miss_index = addShader(miss_shader);
			_shader_groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.pNext = nullptr,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
				.generalShader = miss_index,
				.closestHitShader = VK_SHADER_UNUSED_KHR,
				.anyHitShader = VK_SHADER_UNUSED_KHR,
				.intersectionShader = VK_SHADER_UNUSED_KHR,
				.pShaderGroupCaptureReplayHandle = nullptr,
			});
		}

		_group_begin[static_cast<uint32_t>(ShaderRecordType::HitGroup)] = _shader_groups.size32();
		for (size_t h = 0; h < ci.hit_groups.size(); ++h)
		{
			HitGroup const& hg = ci.hit_groups[h];
			check_opt_shader(hg.closest_hit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
			check_opt_shader(hg.any_hit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
			check_opt_shader(hg.intersection, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
			const uint32_t closest_hit_index = addShader(hg.closest_hit);
			const uint32_t any_hit_index = addShader(hg.any_hit);
			const uint32_t intersection_index = addShader(hg.intersection);
			_shader_groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.pNext = nullptr,
				.generalShader = VK_SHADER_UNUSED_KHR,
				.closestHitShader = closest_hit_index,
				.anyHitShader = any_hit_index,
				.intersectionShader = intersection_index,
				.pShaderGroupCaptureReplayHandle = nullptr,
			});
			VkRayTracingShaderGroupCreateInfoKHR& group = _shader_groups.back();
			if (hg.intersection)
			{
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
			}
			else
			{
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			}
		}

		_group_begin[static_cast<uint32_t>(ShaderRecordType::Callable)] = _shader_groups.size32();
		for (size_t c = 0; c < ci.callables.size(); ++c)
		{
			std::shared_ptr<Shader> const& callable_shader = ci.callables[c];
			check_shader(callable_shader, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
			const uint32_t call_index = addShader(callable_shader);
			_shader_groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
				.pNext = nullptr,
				.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
				.generalShader = call_index,
				.closestHitShader = VK_SHADER_UNUSED_KHR,
				.anyHitShader = VK_SHADER_UNUSED_KHR,
				.intersectionShader = VK_SHADER_UNUSED_KHR,
				.pShaderGroupCaptureReplayHandle = nullptr,
			});
		}

		setInvalidationCallbacks();
	}

	uint32_t RayTracingProgram::addShader(std::shared_ptr<Shader> const& shader)
	{
		uint32_t res = VK_SHADER_UNUSED_KHR;
		if (shader)
		{
			auto it = std::find(_shaders.begin(), _shaders.end(), shader);
			if (it != _shaders.end())
			{
				res = static_cast<uint32_t>(it - _shaders.begin());
			}
			else
			{
				res = _shaders.size32();
				_shaders.push_back(shader);
			}
		}
		return res;
	}

	void RayTracingProgram::createInstanceIFP()
	{
		MyVector<std::shared_ptr<ShaderInstance>> shaders(_shaders.size());
		for (size_t i = 0; i < _shaders.size(); ++i)
		{
			shaders[i] = _shaders[i]->instance();
		}
		_inst = std::make_shared<RayTracingProgramInstance>(RayTracingProgramInstance::CI{
			.app = application(),
			.name = name(),
			.shaders = std::move(shaders),
			.shader_groups = _shader_groups,
			.group_begin = _group_begin,
			.sets_layouts = _provided_sets_layouts,
		});
	}
}