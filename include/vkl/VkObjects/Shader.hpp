#pragma once

#include <vkl/App/VkApplication.hpp>
#include <string>
#include <shaderc/shaderc.hpp>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "DescriptorSetLayout.hpp"
#include "AbstractInstance.hpp"
#include <set>
#include <unordered_map>
#include <vkl/Execution/UpdateContext.hpp>
#include <filesystem>

namespace vkl
{

	enum class ShadingLanguage
	{
		Unknown = 0,
		SPIR_V,
		GLSL,
		Slang,
		HLSL,
	};

	class ShaderInstance : public AbstractInstance
	{
	public:

	protected:

		that::FileSystem::Path _main_path;
		ShadingLanguage _source_language = ShadingLanguage::Unknown;
		
		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		MyVector<uint32_t> _spv_code;
		SpvReflectShaderModule _reflection;
		MyVector<that::FileSystem::Path> _dependencies;
		size_t _shader_string_packed_capacity = 32;

		std::string _preprocessed_source;

		AsynchTask::ReturnType _creation_result;

		struct PreprocessingState
		{
			std::set<that::FileSystem::Path> pragma_once_files = {};
			MyVector<that::FileSystem::Path> include_directories = {};
		};

		enum class IncludeType
		{
			None,
			Quotes,
			Brackets,
		};

		std::string preprocessIncludesAndDefinitions(that::FileSystem::Path const& path, DefinitionsList const& definitions, PreprocessingState& preprocessing_state, size_t recursion_level, IncludeType include_type);

		std::string preprocessStrings(std::string const& glsl);

		bool deduceShadingLanguageIFP(std::string & source);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			that::FileSystem::Path const& source_path = {};
			ShadingLanguage source_language = ShadingLanguage::Unknown;
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			DefinitionsList const& definitions = {};
			size_t shader_string_packed_capacity = 32;
		};
		using CI = CreateInfo;

		ShaderInstance(CreateInfo const&);

		ShaderInstance(ShaderInstance const&) = delete;
		ShaderInstance(ShaderInstance &&) = delete;

		virtual ~ShaderInstance() override;

		ShaderInstance& operator=(ShaderInstance const&) = delete;
		ShaderInstance& operator=(ShaderInstance &&) = delete;



		std::string preprocess(that::FileSystem::Path const& path, DefinitionsList const& definitions);

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

		void createInstance(SpecializationKey const& key, DefinitionsList const& common_definitions, size_t string_packed_capacity);

		virtual void destroyInstanceIFN() override;

		mutable std::shared_ptr<AsynchTask> _create_instance_task = nullptr;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			that::FileSystem::Path const& source_path = {};
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