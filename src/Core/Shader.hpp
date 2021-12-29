#pragma once

#include "VkApplication.hpp"
#include <string>
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <SPIRV-Reflect/spirv_reflect.h>

namespace vkl
{
	std::string readFileToString(std::filesystem::path const& path);

	class Shader : public VkObject
	{
	protected:

		VkShaderStageFlagBits _stage;
		VkShaderModule _module = VK_NULL_HANDLE;
		std::vector<uint32_t> _spv_code;
		SpvReflectShaderModule _reflection;


	public:

		Shader(VkApplication* app, std::filesystem::path const& path, VkShaderStageFlagBits stage);

		Shader(VkApplication * app, std::string const& code, VkShaderStageFlagBits stage);

		Shader(Shader const&) = delete;
		constexpr Shader(Shader&& other) noexcept :
			VkObject(std::move(other)),
			_stage(other._stage),
			_module(other._module),
			_spv_code(other._spv_code),
			_reflection(std::move(other._reflection))
		{
			other._module = VK_NULL_HANDLE;
			other._reflection._internal = nullptr;
		}

		~Shader();

		Shader& operator=(Shader const&) = delete;
		constexpr Shader& operator=(Shader&& other) noexcept
		{
			VkObject::operator=(std::move(other));
			std::swap(_stage, other._stage);
			std::swap(_module, other._module);
			std::swap(_spv_code, other._spv_code);
			std::swap(_reflection, other._reflection);
			return *this;
		}
		
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

		constexpr const SpvReflectShaderModule& reflection()const
		{
			return _reflection;
		}

		std::string entryName()const;

		VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo()const;
	};
}