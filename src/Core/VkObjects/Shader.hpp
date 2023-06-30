#pragma once

#include <Core/App/VkApplication.hpp>
#include <string>
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "DescriptorSetLayout.hpp"
#include "AbstractInstance.hpp"
#include <set>
#include <unordered_map>
#include <Core/Execution/UpdateContext.hpp>

namespace vkl
{
	std::vector<uint8_t> readFile(std::filesystem::path const& path);
	
	std::string readFileToString(std::filesystem::path const& path);

	class ShaderInstance : public AbstractInstance
	{
	protected:
		
		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		std::vector<uint32_t> _spv_code;
		SpvReflectShaderModule _reflection;
		std::vector<std::filesystem::path> _dependencies;
		size_t _shader_string_packed_capacity = 32;

		struct PreprocessingState
		{
			std::set<std::filesystem::path> pragma_once_files = {};
		};

		std::string preprocessIncludesAndDefinitions(std::filesystem::path const& path, std::vector<std::string> const& definitions, PreprocessingState& preprocessing_state);

		std::string preprocessStrings(std::string const& glsl);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			std::vector<std::string> const& definitions = {};
			size_t shader_string_packed_capacity = 32;
		};
		using CI = CreateInfo;

		ShaderInstance(CreateInfo const&);

		ShaderInstance(ShaderInstance const&) = delete;
		ShaderInstance(ShaderInstance &&) = delete;

		virtual ~ShaderInstance() override;

		ShaderInstance& operator=(ShaderInstance const&) = delete;
		ShaderInstance& operator=(ShaderInstance &&) = delete;



		std::string preprocess(std::filesystem::path const& path, std::vector<std::string> const& definitions);

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



		std::filesystem::path _path;
		VkShaderStageFlagBits _stage;
		DynamicValue<std::vector<std::string>> _definitions;
		std::vector<Dependecy> _dependencies;
		std::chrono::file_time<std::chrono::file_clock::duration> _instance_time;

		SpecializationKey _current_key = {};
		SpecializationTable _specializations = {};

		void createInstance(SpecializationKey const& key, std::vector<std::string> const& common_definitions, size_t string_packed_capacity);

		void destroyInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			DynamicValue<std::vector<std::string>> definitions;
		};
		using CI = CreateInfo;

		Shader(CreateInfo const& ci);

		virtual ~Shader() override;

		bool updateResources(UpdateContext & ctx);
		
	};
}