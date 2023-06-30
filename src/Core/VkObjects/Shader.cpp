#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <exception>
#include <cassert>
#include <sstream>
#include <string_view>
#include <iostream>
#include <format>
#include <thread>

namespace vkl
{
	std::vector<uint8_t> readFile(std::filesystem::path const& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Could not open file: " + path.string());
		}

		size_t size = file.tellg();
		std::vector<uint8_t> res;
		res.resize(size);
		file.seekg(0);
		file.read((char*)res.data(), size);
		file.close();
		return res;
	}

	std::string readFileToString(std::filesystem::path const& path)
	{
		std::ifstream file = std::ifstream(path, std::ios::ate | std::ios::binary);
		int tries = 0;
		const int max_tries = 8;
		while (!file.is_open() && tries != max_tries)
		{
			++tries;
			file = std::ifstream(path, std::ios::ate | std::ios::binary);
			std::this_thread::sleep_for(1us);
		}

		if (!file.is_open())
		{
			throw std::runtime_error("Could not open file: " + path.string());
		}

		size_t size = file.tellg();
		std::string res;
		res.resize(size);
		file.seekg(0);
		file.read((char*)res.data(), size);
		file.close();
		return res;
	}
	

	std::string ShaderInstance::preprocessIncludesAndDefinitions(std::filesystem::path const& path, std::vector<std::string> const& definitions, PreprocessingState & preprocessing_state)
	{
		_dependencies.push_back(path);
		std::string content = readFileToString(path);

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

		const auto countLines = [&](size_t b, size_t e)
		{
			return std::count(content.data() + b, content.data() + e, '\n');
		};

		while (true)
		{
			// TODO check if not in comment
			const size_t include_begin = content.find("#include", copied_so_far);
			const size_t pragma_once_begin = content.find("#pragma once", copied_so_far); // TODO regex?

			if (pragma_once_begin != std::string::npos)
			{
				preprocessing_state.pragma_once_files.emplace(path);
				// Comment the #pragma once so the glsl compiler doesn't print a warning
				content[pragma_once_begin] = '/';
				content[pragma_once_begin + 1] = '/';
			}	

			if (std::string::npos != include_begin)
			{

				const size_t line_end = content.find("\n", include_begin);
				const std::string_view line(content.data() + include_begin, line_end - include_begin);

				const size_t path_begin = line.find("\"") + 1;
				const size_t path_end = line.rfind("\"");
				const std::string_view include_path_relative(line.data() + path_begin, path_end - path_begin);

				const std::filesystem::path path_to_include = folder.string() + std::string("/") + std::string(include_path_relative);

				oss << std::string_view(content.data() + copied_so_far, include_begin - copied_so_far);
				
				if (!preprocessing_state.pragma_once_files.contains(path_to_include))
				{
					const std::string included_code = preprocessIncludesAndDefinitions(path_to_include, {}, preprocessing_state);
					oss << "#line 1 " << path_to_include << "\n";
					oss << included_code;
					oss << "\n#line " << (countLines(0, line_end) + 1) << ' ' << path << "\n";
				}
				else
				{
					int _ = 0;
				}
				copied_so_far = line_end;

			}
			else
				break;
		}

		oss << std::string_view(content.data() + copied_so_far, content.size() - copied_so_far);

		return oss.str();
	}
	
	std::string ShaderInstance::preprocessStrings(const std::string& glsl)
	{
		std::stringstream res;

		const auto findNextStringLiteral = [&](size_t from)
		{
			const size_t opening_dq = glsl.find_first_of('"', from);

			if (opening_dq == std::string::npos)
			{
				return ""sv;
			}

			const size_t closing_dq = [&]() {
				size_t begin = opening_dq + 1;
				size_t res = begin; 
				while (true)
				{
					res = glsl.find_first_of('"', begin);
					if (glsl[res - 1] != '\\')
					{
						break;
					}
					begin = res;
				}
				return res;
			}();
			const std::string_view res(glsl.data() + opening_dq, closing_dq - opening_dq + 1);
			return res;
		};

		const auto extractLiteralContent = [](std::string_view lt)
		{
			return std::string_view(lt.data() + 1, lt.size() - 2);
		};

		const auto ignoreStringLiteral = [&](std::string_view literal)
		{
			// Check if literal is in a macro
			const size_t begin = literal.data() - glsl.data();
			const size_t line_begin = glsl.rfind('\n', begin);
			const std::string_view line = std::string_view(glsl.data() + line_begin, literal.size() + (begin - line_begin));
			const size_t error_begin = line.find("#error");
			if (error_begin != std::string::npos)
			{
				return true;
			}
			const size_t macro_line_begin = line.find("#line");
			if (macro_line_begin != std::string::npos)
			{
				return true;
			}
			// TODO check if literal is in a comment, but not necessary
			return false;
		};

		const size_t max_str_len = _shader_string_packed_capacity - 1;

		const auto convertLiteralContentToGLSL = [&](std::string_view lt)
		{
			if (lt.size() > max_str_len)
			{
				VK_LOG << "Warning: string literal \"" << lt << "\" exceeding maximum capacity of " << max_str_len << "!" << std::endl;
				lt = std::string_view(lt.data(), max_str_len); // Crop the literal
			}

			std::vector<uint32_t> chunks((max_str_len + 1) / 4, 0u);
			std::memcpy(chunks.data(), lt.data(), lt.size());
			
			// Set len
			chunks.back() &= 0x00ff'ffff;
			chunks.back() |= (uint32_t(lt.size()) << (3 * 8));

			std::string str_glsl = "makeShaderString(uint32_t[](";
			for (uint32_t c : chunks)
			{
				std::string c_hex = "0x" + std::format("{:x}", c);
				str_glsl += c_hex + ",";
			}
			// Replace the last comma
			str_glsl.back() = ')';
			// Close the "makeShaderString" function call
			str_glsl += ")";
			return str_glsl;
		};

		
		size_t copied_so_far = 0;
		while (true)
		{
			std::string_view lt = findNextStringLiteral(copied_so_far);
			if (lt == ""sv)
			{
				break;
			}
			const size_t lt_begin = lt.data() - glsl.data();
			if (ignoreStringLiteral(lt))
			{
				const std::string_view to_copy = std::string_view(glsl.data() + copied_so_far, lt_begin - copied_so_far + lt.size());
				res << to_copy;
				copied_so_far += to_copy.size();
				continue;
			}

			const std::string_view copied_so_far_view = std::string_view(glsl.data(), copied_so_far);
			const std::string_view lt_content = extractLiteralContent(lt);
			const std::string lt_glsl = convertLiteralContentToGLSL(lt_content);

			const std::string_view to_copy = std::string_view(glsl.data() + copied_so_far, lt_begin - copied_so_far);
			res << to_copy;
			res << lt_glsl;
			copied_so_far = (lt_begin + lt.size());
		}

		res << std::string_view(glsl.data() + copied_so_far, glsl.size() - copied_so_far);

		const std::string _res = res.str();
		return _res;
	}

	std::string ShaderInstance::preprocess(std::filesystem::path const& path, std::vector<std::string> const& definitions)
	{
		PreprocessingState preprocessing_state = {};
		std::string full_source = preprocessIncludesAndDefinitions(path, definitions, preprocessing_state);

		std::string final_source = preprocessStrings(full_source);
		return final_source;
	}

	shaderc_shader_kind getShaderKind(VkShaderStageFlagBits stage)
	{
		shaderc_shader_kind kind = (shaderc_shader_kind)-1;
		switch (stage)
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
		default:
			assert(false);
			break;
		}
		return kind;
	}

	std::string getShaderStageName(VkShaderStageFlagBits stage)
	{
		std::string res = {};
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			res = "VERTEX";
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			res = "FRAGMENT";
			break;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			res = "GEOMETRY";
			break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			res = "TESSELLATION_CONTROL";
			break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			res = "TESSELLATION_EVALUATION";
			break;
		case VK_SHADER_STAGE_TASK_BIT_NV:
			res = "TASK";
			break;
		case VK_SHADER_STAGE_MESH_BIT_NV:
			res = "MESH";
			break;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			res = "COMPUTE";
			break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			res = "RAYGEN";
			break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
			res = "INTERSECTION";
			break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
			res = "ANY_HIT";
			break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			res = "CLOSEST_HIT";
			break;
		case VK_SHADER_STAGE_MISS_BIT_KHR:
			res = "MISS";
			break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
			res = "CALLABLE";
			break;
		default:
			assert(false);
			break;
		}
		return res;
	}

	bool ShaderInstance::compile(std::string const& code, std::string const& filename)
	{
		shaderc::Compiler compiler;
		shaderc::CompilationResult res = compiler.CompileGlslToSpv(code, getShaderKind(_stage), filename.c_str());
		size_t errors = res.GetNumErrors();

		if (errors)
		{
			std::cerr << res.GetErrorMessage() << std::endl;
			return false;
		}
		_spv_code = std::vector<uint32_t>(res.cbegin(), res.cend());
		VkShaderModuleCreateInfo module_ci{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = _spv_code.size() * sizeof(uint32_t),
			.pCode = _spv_code.data(),
		};
		VK_CHECK(vkCreateShaderModule(_app->device(), &module_ci, nullptr, &_module), "Failed to create a shader module.");

		return true;
	}

	void ShaderInstance::reflect()
	{
		spvReflectDestroyShaderModule(&_reflection);
		spvReflectCreateShaderModule(_spv_code.size() * sizeof(uint32_t), _spv_code.data(), &_reflection);
	}

	ShaderInstance::ShaderInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_stage(ci.stage),
		_reflection(std::zeroInit(_reflection)),
		_shader_string_packed_capacity(ci.shader_string_packed_capacity)
	{
		using namespace std::containers_operators;
		std::filesystem::file_time_type compile_time = std::filesystem::file_time_type::min();

		VK_LOG << "Compiling: " << ci.source_path << "\n";

		// Try to compile while it fails
		size_t attempt = 0;
		while (true)
		{
			++attempt;
			const std::filesystem::file_time_type update_time = [&]() {
				std::filesystem::file_time_type res = std::filesystem::file_time_type::min();
				for (const auto& dep : _dependencies)
				{
					std::filesystem::file_time_type ft = std::filesystem::last_write_time(dep);
					res = std::max(res, ft);
				}
				return res;
			}();

			if (update_time >= compile_time)
			{
				Sleep(1);
				_dependencies.clear();
				std::string semantic_definition = "SHADER_SEMANTIC_" + getShaderStageName(_stage) + " 1";
				std::vector<std::string> defines = { semantic_definition };
				defines += ci.definitions;

				std::string preprocessed = preprocess(ci.source_path, defines);
				
				if (preprocessed == "")
				{
					continue;
				}
				bool res = compile(preprocessed, ci.source_path.string());
				compile_time = std::chrono::file_clock::now();
				if (res)
				{
					break;
				}
				else
				{
					int _ = 0;
				}
			}
		}
		reflect();
	}

	ShaderInstance::~ShaderInstance()
	{
		//This function sets the module to null
		spvReflectDestroyShaderModule(&_reflection);

		if (_module)
		{
			callDestructionCallbacks();
			vkDestroyShaderModule(_app->device(), _module, nullptr);
			_module = VK_NULL_HANDLE;
		}
	}

	std::string ShaderInstance::entryName()const
	{
		return std::string(_reflection.entry_point_name);
	}

	VkPipelineShaderStageCreateInfo ShaderInstance::getPipelineShaderStageCreateInfo()const
	{
		return VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = _stage,
			.module = module(),
			.pName = _reflection.entry_point_name,
		};
	}

	void Shader::createInstance(SpecializationKey const& key, std::vector<std::string> const& common_definitions, size_t string_packed_capacity)
	{
		if (_specializations.contains(key))
		{
			_inst = _specializations[key];
		}
		else {
			using namespace std::containers_operators;
			std::vector<std::string> definitions = (*_definitions);
			definitions += common_definitions;
			_inst = std::make_shared<ShaderInstance>(ShaderInstance::CI{
				.app = application(),
				.name = name(),
				.source_path = _path,
				.stage = _stage,
				.definitions = definitions,
				.shader_string_packed_capacity = string_packed_capacity,
				});
			_specializations[key] = _inst;
			_instance_time = std::chrono::file_clock::now();
		}

		_dependencies = _inst->dependencies();
	}

	void Shader::destroyInstance()
	{
		if (_inst)
		{
			callInvalidationCallbacks();
			_inst = nullptr;
		}
	}

	bool Shader::updateResources(UpdateContext & ctx)
	{
		using namespace std::containers_operators;
		bool res = false;

		std::vector<std::string> definitions = *_definitions;
		definitions += ctx.commonDefinitions().collapsed();
		SpecializationKey new_key;
		new_key.definitions = std::accumulate(definitions.begin(), definitions.end(), ""s, [](std::string const& a, std::string const& b)
		{
			return a + "\n"s + b;
		});
		
		if (ctx.checkShaders())
		{
			for (const auto& dep : _dependencies)
			{
				const std::filesystem::file_time_type new_time = std::filesystem::last_write_time(dep);
				if (new_time > _instance_time)
				{
					_specializations.clear();
					res = true;
				}
			}
		}
		
		const bool use_different_spec = new_key != _current_key;
		if (use_different_spec)
		{
			_current_key = new_key;
			res = true;
		}

		if (res)
		{
			destroyInstance();
		}
		
		
		if (!_inst)
		{
			std::string capacity = ctx.commonDefinitions().getDefinition("SHADER_STRING_CAPACITY");
			int packed_capcity = capacity.empty() ? 32 : std::atoi(capacity.c_str());
			createInstance(_current_key, ctx.commonDefinitions().collapsed(), static_cast<size_t>(packed_capcity));
			res = true;
		}

		return res;
	}

	Shader::Shader(CreateInfo const& ci):
		ParentType(ci.app, ci.name),
		_path(ci.source_path),
		_stage(ci.stage),
		_definitions(ci.definitions)
	{
		_instance_time = std::filesystem::file_time_type::min();
	}

	Shader::~Shader()
	{
		destroyInstance();
	}
}