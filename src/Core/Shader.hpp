#pragma once

#include "VkApplication.hpp"
#include <string>
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <SPIRV-Reflect/spirv_reflect.h>
#include "DescriptorSetLayout.hpp"
#include "AbstractInstance.hpp"

namespace vkl
{
	std::vector<uint8_t> readFile(std::filesystem::path const& path);
	
	std::string readFileToString(std::filesystem::path const& path);

	class ShaderInstance : public VkObject
	{
	protected:
		
		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		std::vector<uint32_t> _spv_code;
		SpvReflectShaderModule _reflection;
		std::vector<std::filesystem::path> _dependencies;


	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			std::vector<std::string> const& definitions = {};
		};
		using CI = CreateInfo;

		ShaderInstance(CreateInfo const&);

		ShaderInstance(ShaderInstance const&) = delete;
		ShaderInstance(ShaderInstance &&) = delete;

		virtual ~ShaderInstance() override;

		ShaderInstance& operator=(ShaderInstance const&) = delete;
		ShaderInstance& operator=(ShaderInstance &&) = delete;

		std::string preprocess(std::filesystem::path const& path, std::vector<std::string> const& definitions);

		void compile(std::string const& code, std::string const& filename = "");

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
	protected:

		using ParentType = InstanceHolder<ShaderInstance>;

		using Dependecy = std::filesystem::path;

		std::filesystem::path _path;
		VkShaderStageFlagBits _stage;
		std::vector<std::string> _definitions;
		std::vector<Dependecy> _dependencies;
		std::chrono::file_time<std::chrono::file_clock::duration> _instance_time;

		void createInstance();

		void destroyInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::filesystem::path const& source_path = {};
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			std::vector<std::string> const& definitions = {};
		};
		using CI = CreateInfo;

		Shader(CreateInfo const& ci);

		Shader(Shader && other) noexcept : 
			ParentType(std::move(other)),
			_path(std::move(other._path)),
			_stage(other._stage),
			_definitions(std::move(other._definitions)),
			_dependencies(std::move(other._dependencies)),
			_instance_time(std::move(other._instance_time))
		{}
		
		Shader& operator=(Shader&& other) noexcept
		{
			ParentType::operator=(std::move(other));
			std::swap(_path, other._path);
			std::swap(_stage, other._stage);
			std::swap(_definitions, other._definitions);
			std::swap(_dependencies, other._dependencies);
			std::swap(_instance_time, other._instance_time);
			return *this;
		}

		virtual ~Shader() override;

		bool updateResources();
		
	};
}