#pragma once

#include <Core/App/VkApplication.hpp>
#include <string>
#include <shaderc/shaderc.hpp>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "DescriptorSetLayout.hpp"
#include "AbstractInstance.hpp"
#include <set>
#include <unordered_map>
#include <Core/Execution/UpdateContext.hpp>
#include <filesystem>

namespace vkl
{

	class ShaderInstance : public AbstractInstance
	{
	protected:

		std::filesystem::path _main_path;
		
		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		std::vector<uint32_t> _spv_code;
		SpvReflectShaderModule _reflection;
		std::vector<std::filesystem::path> _dependencies;
		size_t _shader_string_packed_capacity = 32;

		std::string _preprocessed_source;

		AsynchTask::ReturnType _creation_result;

		struct PreprocessingState
		{
			std::set<std::filesystem::path> pragma_once_files = {};
			const MountingPoints * mounting_points;
		};

		std::string preprocessIncludesAndDefinitions(std::filesystem::path const& path, DefinitionsList const& definitions, PreprocessingState& preprocessing_state, size_t recursion_level);

		std::string preprocessStrings(std::string const& glsl);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			DefinitionsList const& definitions = {};
			size_t shader_string_packed_capacity = 32;
			const MountingPoints * mounting_points = nullptr;
		};
		using CI = CreateInfo;

		ShaderInstance(CreateInfo const&);

		ShaderInstance(ShaderInstance const&) = delete;
		ShaderInstance(ShaderInstance &&) = delete;

		virtual ~ShaderInstance() override;

		ShaderInstance& operator=(ShaderInstance const&) = delete;
		ShaderInstance& operator=(ShaderInstance &&) = delete;



		std::string preprocess(std::filesystem::path const& path, DefinitionsList const& definitions, const MountingPoints * mounting_points);

		bool compile(std::string const& code, std::string const& filename = "");

		void reflect();

		constexpr VkShaderModule module() const
		{
			return _module;
		}

		constexpr auto handle()const
		{
			return module();
		}

		constexpr operator VkShaderModule()const
		{
			return _module;
		}

		constexpr VkShaderStageFlagBits stage()const
		{
			return _stage;
		}

		constexpr const SpvReflectShaderModule& reflection()const
		{
			return _reflection;
		}

		std::string entryName()const;

		std::vector<std::filesystem::path> const& dependencies()const
		{
			return _dependencies;
		}

		VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo()const;

		const AsynchTask::ReturnType& getCreationResult()const
		{
			return _creation_result;
		}
	};

	class Shader : public InstanceHolder<ShaderInstance>
	{
	public:
		
		struct SpecializationKey
		{
			std::string definitions = {};

			bool operator==(SpecializationKey const& o) const
			{
				return definitions == o.definitions;
			}

			bool operator!=(SpecializationKey const& o) const
			{
				return definitions != o.definitions;
			}

			void clear()
			{
				definitions.clear();
			}
		};

		struct SpecKeyHasher
		{
			size_t operator()(SpecializationKey const& key) const
			{
				return std::hash<std::string>()(key.definitions);
			}
		};
		using SpecializationTable = std::unordered_map<SpecializationKey, std::shared_ptr<ShaderInstance>, SpecKeyHasher>;

	protected:

		using ParentType = InstanceHolder<ShaderInstance>;

		using Dependecy = std::filesystem::path;

		size_t _update_tick = 0;
		bool _latest_update_result = false;
		size_t _check_tick = 0;
		std::filesystem::path _path;
		VkShaderStageFlagBits _stage;
		DynamicValue<DefinitionsList> _definitions;
		std::vector<Dependecy> _dependencies;
		std::chrono::file_time<std::chrono::file_clock::duration> _instance_time;

		SpecializationKey _current_key = {};
		SpecializationTable _specializations = {};

		void createInstance(SpecializationKey const& key, DefinitionsList const& common_definitions, size_t string_packed_capacity, const MountingPoints * mounting_points);

		virtual void destroyInstanceIFN() override;

		mutable std::shared_ptr<AsynchTask> _create_instance_task = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			DynamicValue<DefinitionsList> definitions;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		Shader(CreateInfo const& ci);

		virtual ~Shader() override;

		bool updateResources(UpdateContext & ctx);

		bool hasInstanceOrIsPending() const
		{
			return _inst || _create_instance_task;
		}
		
		void waitForInstanceCreationIFN();

		std::shared_ptr<ShaderInstance> getInstanceWaitIFN();

		const std::shared_ptr<AsynchTask>& compileTask()const
		{
			return _create_instance_task;
		}

		constexpr VkShaderStageFlagBits stage() const
		{
			return _stage;
		}
	};
}