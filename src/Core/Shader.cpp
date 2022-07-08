#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <exception>
#include <cassert>
#include <sstream>
#include <string_view>
#include <iostream>

namespace vkl
{
	std::string readFileToString(std::filesystem::path const& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Could not open file: " + path.string());
		}

		size_t size = file.tellg();
		std::string res;
		res.resize(size);
		file.seekg(0);
		file.read(res.data(), size);
		file.close();
		return res;

	}

	std::string Shader::preprocess(std::filesystem::path const& path, std::vector<std::string> const& definitions)
	{
		const std::string content = readFileToString(path);
		std::stringstream oss;

		const std::filesystem::path folder = path.parent_path();
		
		size_t copied_so_far = 0;

		if (!definitions.empty())
		{
			const size_t version_begin = content.find("#version", copied_so_far);
			assert(version_begin < content.size());
			const size_t version_end = content.find("\n", version_begin);

			oss << std::string_view(content.data() + copied_so_far, version_end - copied_so_far);
			copied_so_far = version_end;
			for (const std::string& def : definitions)
			{
				oss << "#define " << def << "\n";
			}
		}

		while (true)
		{
			const size_t include_begin = content.find("#include", copied_so_far);

			if (std::string::npos != include_begin)
			{

				const size_t line_end = content.find("\n", include_begin);
				const std::string_view line(content.data() + include_begin, line_end - include_begin);

				const size_t path_begin = line.find("\"") + 1;
				const size_t path_end = line.rfind("\"");
				const std::string_view include_path_relative(line.data() + path_begin, path_end - path_begin);

				const std::filesystem::path path_to_include = folder.string() + std::string("/") + std::string(include_path_relative);

				const std::string included_code = preprocess(path_to_include, {});

				oss << std::string_view(content.data() + copied_so_far, include_begin - copied_so_far);
				oss << included_code;
				copied_so_far = line_end;

			}
			else
				break;
		}

		oss << std::string_view(content.data() + copied_so_far, content.size() - copied_so_far);

		return oss.str();
	}

	shaderc_shader_kind getShaderKind(VkShaderStageFlagBits stage)
	{
		shaderc_shader_kind kind;
		switch(stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			kind = shaderc_vertex_shader;
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			kind = shaderc_fragment_shader;
			break;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			kind = shaderc_geometry_shader;
			break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			kind = shaderc_tess_control_shader;
			break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			kind = shaderc_tess_evaluation_shader;
			break;
		case VK_SHADER_STAGE_TASK_BIT_NV:
			kind = shaderc_task_shader;
			break;
		case VK_SHADER_STAGE_MESH_BIT_NV:
			kind = shaderc_mesh_shader;
			break;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			kind = shaderc_compute_shader;
			break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			kind = shaderc_raygen_shader;
			break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
			kind = shaderc_intersection_shader;
			break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
			kind = shaderc_anyhit_shader;
			break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			kind = shaderc_closesthit_shader;
			break;
		case VK_SHADER_STAGE_MISS_BIT_KHR:
			kind = shaderc_miss_shader;
			break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
			kind = shaderc_callable_shader;
			break;
		}
		return kind;
		
	}

	void Shader::compile(std::string const& code, std::string const& filename)
	{
		shaderc::Compiler compiler;
		shaderc::CompilationResult res = compiler.CompileGlslToSpv(code, getShaderKind(_stage), filename.c_str());
		size_t errors = res.GetNumErrors();
		
		if (errors)
		{
			std::cerr << res.GetErrorMessage() << std::endl;
			throw(std::runtime_error(res.GetErrorMessage()));
		}
		_spv_code = std::vector<uint32_t>(res.cbegin(), res.cend());
		VkShaderModuleCreateInfo module_ci{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = _spv_code.size() * sizeof(uint32_t),
			.pCode = _spv_code.data(),
		};
		VK_CHECK(vkCreateShaderModule(_app->device(), &module_ci, nullptr, &_module), "Failed to create a shader module.");	
	}

	void Shader::reflect()
	{
		spvReflectDestroyShaderModule(&_reflection);
		spvReflectCreateShaderModule(_spv_code.size() * sizeof(uint32_t), _spv_code.data(), &_reflection);
	}

	Shader::Shader(VkApplication* app, std::filesystem::path const& path, VkShaderStageFlagBits stage, std::vector<std::string> const& definitions) :
		VkObject(app),
		_stage(stage),
		_reflection(std::zeroInit(_reflection))
	{
		compile(preprocess(path, definitions), path.string());
		reflect();
	}
	
	Shader::~Shader()
	{
		//This function sets the module to null
		spvReflectDestroyShaderModule(&_reflection);

		if (_module)
		{
			vkDestroyShaderModule(_app->device(), _module, nullptr);
			_module = VK_NULL_HANDLE;
		}
	}

	std::string Shader::entryName()const
	{
		return std::string(_reflection.entry_point_name);
	}

	VkPipelineShaderStageCreateInfo Shader::getPipelineShaderStageCreateInfo()const
	{
		return VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = _stage,
			.module = module(),
			.pName = _reflection.entry_point_name,
		};
	}
}