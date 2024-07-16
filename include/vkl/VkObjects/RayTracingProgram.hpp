#pragma once

#include <vkl/VkObjects/Program.hpp>

namespace vkl
{
	enum class ShaderRecordType
	{
		RayGen = 0,
		Miss = 1,
		HitGroup = 2,
		Callable = 3,
		MAX_ENUM,
	};

	class RayTracingProgramInstance : public ProgramInstance
	{
	public:

	protected:

		MyVector<VkRayTracingShaderGroupCreateInfoKHR> _shader_groups = {};
		std::array<uint32_t, 4> _group_begin = {0, 0, 0, 0};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			MyVector<std::shared_ptr<ShaderInstance>> shaders = {};
			MyVector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups = {};
			std::array<uint32_t, 4> group_begin = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
		};
		using CI = CreateInfo;

		RayTracingProgramInstance(CreateInfo const& ci);

		virtual ~RayTracingProgramInstance() override = default;

		MyVector<VkRayTracingShaderGroupCreateInfoKHR> const& shaderGroups()const
		{
			return _shader_groups;
		}

		uint32_t raygenBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::RayGen)];
		}

		uint32_t missBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::Miss)];
		}

		uint32_t hitGroupBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::HitGroup)];
		}

		uint32_t callableBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::Callable)];
		}

		uint32_t getGroupBeginIndex(ShaderRecordType t) const
		{
			return getGroupBeginIndex(static_cast<uint32_t>(t));
		}

		uint32_t getGroupBeginIndex(uint32_t shader_record_type) const
		{
			assert(shader_record_type < _group_begin.size());
			return _group_begin[shader_record_type];
		}
	};

	class RayTracingProgram : public Program
	{
	public:

		struct HitGroup
		{
			std::shared_ptr<Shader> closest_hit = nullptr;
			std::shared_ptr<Shader> any_hit = nullptr;
			std::shared_ptr<Shader> intersection = nullptr;
		};

	protected:

		// Can we have multiple RayGen in one RT Pipeline? (and select which one to call during vkCmdTraceRaysKHR)
		// RayGen is stored in _shaders[0]
		// Then the miss shaders
		// Then the different hit group shaders
		// Then the callable shaders
		MyVector<VkRayTracingShaderGroupCreateInfoKHR> _shader_groups = {};
		std::array<uint32_t, 4> _group_begin = {0, 0, 0, 0};

		uint32_t addShader(std::shared_ptr<Shader> const& shader);

		virtual void createInstanceIFP() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Shader> raygen = nullptr;
			MyVector<std::shared_ptr<Shader>> misses = {};
			MyVector<HitGroup> hit_groups = {};
			MyVector<std::shared_ptr<Shader>> callables = {};
			MultiDescriptorSetsLayouts sets_layouts;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		RayTracingProgram(CreateInfo const& ci);

		virtual ~RayTracingProgram() override = default;

		uint32_t raygenBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::RayGen)];
		}

		uint32_t missBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::Miss)];
		}

		uint32_t hitGroupBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::HitGroup)];
		}

		uint32_t callableBeginIndex() const
		{
			return _group_begin[static_cast<uint32_t>(ShaderRecordType::Callable)];
		}

		uint32_t getGroupBeginIndex(ShaderRecordType t) const
		{
			return getGroupBeginIndex(static_cast<uint32_t>(t));
		}

		uint32_t getGroupBeginIndex(uint32_t shader_record_type) const
		{
			assert(shader_record_type < _group_begin.size());
			return _group_begin[shader_record_type];
		}
	};
}