#pragma once

#include "VkApplication.hpp"
#include <string>
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include "DescriptorSetLayout.hpp"
#include <spirv_cross.hpp>

namespace vkl
{
	std::string readFileToString(std::filesystem::path const& path);

	class Shader : public VkObject
	{
	protected:

		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		std::vector<uint32_t> _spv_code;
		//std::string _glsl_code;
		spirv_cross::Compiler * _reflector = nullptr;
		spirv_cross::EntryPoint _entry_point;


	public:

		Shader(VkApplication* app, std::filesystem::path const& path, VkShaderStageFlagBits stage, std::vector<std::string> const& definitions = {});

		Shader(Shader const&) = delete;
		
		Shader(Shader&& other) noexcept;

		~Shader();

		Shader& operator=(Shader const&) = delete;

		Shader& operator=(Shader&& other) noexcept;

		std::string preprocess(std::filesystem::path const& path, std::vector<std::string> const& definitions);
		
		void compile(std::string const& code, std::string const& filename="");

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

		constexpr const auto& reflector()const
		{
			return *_reflector;
		}

		std::string entryName()const;

		VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo()const;

	};
}