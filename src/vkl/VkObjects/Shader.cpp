#include <vkl/VkObjects/Shader.hpp>

#include <iostream>
#include <fstream>
#include <exception>
#include <cassert>
#include <sstream>
#include <string_view>
#include <format>
#include <thread>

#include <slang/slang.h>
#include <slang/slang-gfx.h>

#define SWITCH_CASE_CONVERT(CASE, VALUE, target) \
case CASE: \
	target = VALUE; \
break;

#define SWITCH_CASE_CONVERT2(CASE, VALUE) SWITCH_CASE_CONVERT(CASE, VALUE, res)

namespace vkl
{
	std::string ShaderInstance::preprocessIncludesAndDefinitions(that::FileSystem::Path const& path, DefinitionsList const& definitions, PreprocessingState & preprocessing_state, size_t recursion_level, IncludeType include_type)
	{
		std::string content;
		{
			that::FileSystem & fs = *application()->fileSystem();
			that::ResultAnd<that::FileSystem::Path> resolved_path = fs.resolve(path);
			if (resolved_path.result != that::Result::Success)
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: Could not parse file path"s,
					.error_message = std::format(
						"Error while preprocessing shader : Could not parse file path : {} \n"
						"Returned code : {} (code: {})\n"
						"Main Shader: {}\n",
						path.string(), that::GetResultStr(resolved_path.result), static_cast<uint64_t>(resolved_path.result), _main_path.string()
					),
				};
				return std::string();
			}
			that::FileSystem::Path full_path = resolved_path.value;
			const auto validate_path = [&]() -> bool
			{
				return (fs.checkFileExists(full_path, that::FileSystem::Hint::PathIsNative) == that::Result::Success);
			};
			bool path_is_valid = false;
			full_path = resolved_path.value;
			path_is_valid = validate_path();
			if (include_type == IncludeType::Brackets)
			{
				if (!path_is_valid)
				{
					for (const auto& include_dir : preprocessing_state.include_directories)
					{
						full_path = include_dir / resolved_path.value;
						path_is_valid = validate_path();
						if (path_is_valid)
						{
							break;
						}
					}
				}
			}

			if (path_is_valid)
			{
				_dependencies.push_back(full_path);
				that::Result read_result = application()->fileSystem()->readFile(that::FileSystem::ReadFileInfo{
					.hint = that::FileSystem::Hint::PathIsNative,
					.path = &full_path,
					.result_string = &content,
				});
				if (read_result != that::Result::Success)
				{
					_creation_result = AsynchTask::ReturnType{
						.success = false,
						.can_retry = true,
						.error_title = "Shader Compilation Error: Could not read file"s,
						.error_message = std::format(
							"Error while preprocessing shader : Could not read file : {} \n"
							"Returned code : {} (code: {})\n"
							"Main Shader: {}\n",
							path.string(), that::GetResultStr(read_result), static_cast<uint64_t>(read_result), _main_path.string()
						),
					};
					return std::string();
				}
			}
			else
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: Invalid file path"s,
					.error_message = std::format(
						"Error while preprocessing shader : Could not find file : {} \n"
						"Resolved path: {}\n"
						"Main Shader: {}\n",
						path.string(), resolved_path.value.string(), _main_path.string()
					),
				};
				return std::string();
			}
			
		}

		std::stringstream oss;

		size_t copied_so_far = 0;

		const auto countLines = [&](size_t b, size_t e)
		{
			return std::count(content.data() + b, content.data() + e, '\n');
		};

		if (!definitions.empty())
		{
			// TODO temporary hack
			if (_source_language != ShadingLanguage::Slang)
			{
			const size_t version_begin = content.find("#version", copied_so_far);
			if (!(version_begin < content.size()))
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: No declared #version"s,
					.error_message =
						"Error while preprocessing shader: No declared #version \n"s +
						"Main Shader: "s + _main_path.string() + ":\n"s +
						"In file "s + path.string() + ":\n"s,
				};
				return std::string();
			}
			const size_t version_end = content.find("\n", version_begin);

			oss << std::string_view(content.data() + copied_so_far, version_end - copied_so_far);
			copied_so_far = version_end;
			}
			for (size_t i = 0; i < definitions.size(); ++i)
			{
				oss << "#define " << definitions[i] << "\n";
			}
			oss << "#line " << (countLines(0, copied_so_far) + 1) << ' ' << path << "\n";
		}


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
				const std::string_view include_line(content.data() + include_begin, line_end - include_begin);

				const auto [path_to_include, include_type] = [&]() -> std::pair<that::FileSystem::Path, IncludeType>
				{
					const size_t rel_path_begin = include_line.find("\"") + 1;
					const size_t rel_path_end = include_line.rfind("\"");

					const size_t mp_path_begin = include_line.find("<") + 1;
					const size_t mp_path_end = include_line.rfind(">");

					const auto validate = [&](size_t begin, size_t end)
					{
						bool res = (end - begin) < include_line.size();
						res &= (begin != std::string::npos);
						res &= (end != std::string::npos);
						return res;
					};

					if(validate(rel_path_begin, rel_path_end))
					{
						const that::FileSystem::Path folder = path.parent_path();
						const std::string_view include_path_relative(include_line.data() + rel_path_begin, rel_path_end - rel_path_begin);
						const that::FileSystem::Path path_to_include = folder.string() + ("/"s + std::string(include_path_relative));
						return {path_to_include, IncludeType::Quotes};
					}
					else if(validate(mp_path_begin, mp_path_end))
					{
						that::FileSystem & fs = *application()->fileSystem();
						const std::string_view mp_path_view(include_line.data() + mp_path_begin, mp_path_end - mp_path_begin);
						const that::FileSystem::Path mp_path = mp_path_view;
						const that::ResultAnd<that::FileSystem::PathStringView> result_mounting_point = fs.ExtractMountingPoint(mp_path);
						const that::FileSystem::PathStringView & mounting_point = result_mounting_point.value;
						const bool path_is_valid = (result_mounting_point.result == that::Result::Success) && (mounting_point.empty() || fs.mountingPointIsNative(mounting_point) || fs.knowsMountingPoint(mounting_point));
						if (path_is_valid)
						{
							return {mp_path, IncludeType::Brackets};
						}
						else
						{
							size_t line_index = countLines(0, line_end);
							_creation_result = AsynchTask::ReturnType{
								.success = false,
								.can_retry = true,
								.error_title = "Shader Compilation Error: Preprocess Includes",
								.error_message = std::format(
									"Error while preprocessing shader: Inclusion error \n"
									"Main Shader: {}\n" 
									// TODO format as a clickable link
									"In file: {}\n"
									"Line: {}\n"
									"Incorrect mp path in #include directive: {}\n",
									_main_path.string(), path.string(), line_index, include_line
								),	
							};
							return {that::FileSystem::Path(), IncludeType::None};
						}
					}
					else
					{
						size_t line_index = countLines(0, line_end);
						_creation_result = AsynchTask::ReturnType{
							.success = false,
							.can_retry = true,
							.error_title = "Shader Compilation Error: Preprocess Includes",
							.error_message = std::format(
								"Error while preprocessing shader: Inclusion error \n"
								"Main Shader: {}\n"
								// TODO format as a clickable link
								"In file: {}\n"
								"Line: {}\n"
								"Could not parse #include directive: {}\n",
								_main_path.string(), path.string(), line_index, include_line
							),
						};
						return { that::FileSystem::Path(), IncludeType::None };
					}
				}();
				if (_creation_result.success == false)
				{
					return {};
				}

				oss << std::string_view(content.data() + copied_so_far, include_begin - copied_so_far);
				
				if (!preprocessing_state.pragma_once_files.contains(path_to_include))
				{
					const std::string included_code = preprocessIncludesAndDefinitions(path_to_include, {}, preprocessing_state, recursion_level + 1, include_type);
					if (_creation_result.success == false)
					{
						size_t line_index = countLines(0, line_end);
						if (_creation_result.error_message.empty())
						{
							_creation_result = AsynchTask::ReturnType{
								.success = false,
								.can_retry = true,
								.error_title = "Shader Compilation Error: Preprocess Includes"s,
								.error_message = std::format(
									"Error while preprocessing shader: Inclusion error \n",
									"Main Shader: {}\n"
									// TODO format as a clickable link
									"In file: {}\n"
									"Line: {}\n"
									"Could not include: {}",
									_main_path.string(), path.string(), line_index, path_to_include.string()
								),
							};
						}
						else
						{
							_creation_result.error_message += std::format(
								"\n"
								"While processing include directive: \n"
								"In file: {}\n"
								"Line: {}\n"
								"{}\n",
								path.string(), line_index, include_line
							);
						}
						return {};
					}
					else
					{
						oss << "#line 1 " << path_to_include << "\n";
						oss << included_code;
						oss << "\n#line " << (countLines(0, line_end) + 1) << ' ' << path << "\n";
					}
				}
				else
				{
					VKL_BREAKPOINT_HANDLE;
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
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: Preprocess Strings"s,
					.error_message = 
						"Error while preprocessing shader: \n"s 
						"Main Shader: "s + _main_path.string() + ":\n"s +
						"String literal \""s + std::string(lt) + "\"\n length of "s + std::to_string(lt.size()) + " exceeding maximum capacity of " + std::to_string(max_str_len),
				};
				return std::string();
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

			if (_creation_result.success == false)
			{
				return {};
			}

			const std::string_view to_copy = std::string_view(glsl.data() + copied_so_far, lt_begin - copied_so_far);
			res << to_copy;
			res << lt_glsl;
			copied_so_far = (lt_begin + lt.size());
		}

		res << std::string_view(glsl.data() + copied_so_far, glsl.size() - copied_so_far);

		const std::string _res = res.str();
		return _res;
	}
		
	static const std::set<const char*> glsl_extensions = []()
	{
		std::set<const char*> res = { ".glsl"};//, ".comp", ".vert", ".tesc", ".tese", ".geom", ".frag", ".task", ".mesh", ".rgen", ".rint", ".rahit", ".rchit", ".rmiss", ".rcall" };
		return res;
	}();
	
	static const std::set<const char*> slang_extensions = []()
	{
		std::set<const char*> res = {".slang"};
		return res;
	}();
	
	bool ShaderInstance::deduceShadingLanguageIFP(std::string& source)
	{
		if (_source_language == ShadingLanguage::Unknown)
		{
			size_t pos = source.find("#version");
			if (pos != std::string::npos)
			{
				// TODO check that the macro is in an active line
				_source_language = ShadingLanguage::GLSL;
			}
		}

		if (_source_language == ShadingLanguage::Unknown)
		{
			// Scan the source for a "#lang glsl/slang/hlsl"
			size_t scanned_so_far = 0;
			while (true)
			{
				size_t pos = source.find("#lang ", scanned_so_far);
				// TODO
				NOT_YET_IMPLEMENTED;
				if (pos == std::string::npos)
				{
					break;
				}

			}
		}
		return _source_language != ShadingLanguage::Unknown;
	}
	
	std::string ShaderInstance::preprocess(that::FileSystem::Path const& path, DefinitionsList const& definitions)
	{
		PreprocessingState preprocessing_state = {
			.include_directories = application()->includeDirectories(),
		};
		_dependencies.clear();
		std::string full_source;

		if (_source_language == ShadingLanguage::Unknown)
		{
			const that::FileSystem::Path ext = path.extension();
			if (ext == ".spv")
			{
				_source_language = ShadingLanguage::SPIR_V;
			}
			else if (ext == ".glsl")
			{
				_source_language = ShadingLanguage::GLSL;
			}
			else if (ext == ".slang")
			{
				_source_language = ShadingLanguage::Slang;
			}
			else if (ext == ".hlsl")
			{
				_source_language = ShadingLanguage::HLSL;
			}
			else
			{
				// Still unknown!
				_source_language = ShadingLanguage::Unknown;
			}
		}

		try
		{
			full_source = preprocessIncludesAndDefinitions(path, definitions, preprocessing_state, 0, IncludeType::None);
		}
		catch (std::exception const& e)
		{
			if (e.what() == "string too long"s)
			{
				// There appears to be a bug in the includer:
				// When the last line of the of the file #include <path>, this exception is thrown
				// TODO fix it
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: Preprocess Includes"s,
					.error_message =
						"Error while preprocessing shader: "s + _main_path.string() + "\n"s + 
						"Bug in the includer : Check that a file does not finish by an #include directive (add a new line after to fix)",
				};
			}
			else
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: Preprocess Includes"s,
					.error_message = std::format(
						"Error while preprocessing shader: {}\n"
						"Error: {}",
						_main_path.string(), e.what()
					),
				};
			}
		}

		if (_creation_result.success == false)
		{
			if (_creation_result.error_message.empty())
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = false,
					.error_title = "Shader Compilation Error: Preprocess Includes"s,
					.error_message =
						"Error while preprocessing shader: \n"s +
						"Could not read main shader: " + _main_path.string() + "\n"s +
						full_source,
				};
			}
			return {};
		}

		if (!deduceShadingLanguageIFP(full_source))
		{
			_creation_result = AsynchTask::ReturnType{
				.success = false,
				.can_retry = true,
				.error_title = "Unknown shading language",
				.error_message = 
					"Could not deduce shading language of shader:\n" + _main_path.string(),
			};
		}
		
		std::string final_source;
		if (_source_language == ShadingLanguage::GLSL)
		{
			final_source = preprocessStrings(full_source);
			if (_creation_result.success == false)
			{
			
			}
		}
		else
		{
			final_source = std::move(full_source);
		}
		return final_source;
	}

	constexpr shaderc_shader_kind getShaderKind(VkShaderStageFlagBits stage)
	{
		shaderc_shader_kind res = (shaderc_shader_kind)-1;
		switch (stage)
		{
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_VERTEX_BIT, shaderc_vertex_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, shaderc_tess_control_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, shaderc_tess_evaluation_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_geometry_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_fragment_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_COMPUTE_BIT, shaderc_compute_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TASK_BIT_EXT, shaderc_task_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MESH_BIT_EXT, shaderc_mesh_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_RAYGEN_BIT_KHR, shaderc_raygen_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, shaderc_anyhit_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, shaderc_closesthit_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MISS_BIT_KHR, shaderc_miss_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, shaderc_intersection_shader);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CALLABLE_BIT_KHR, shaderc_callable_shader);
		}
		return res;
	}

	std::string_view getShaderStageName(VkShaderStageFlagBits stage)
	{
		std::string_view res = {};
		switch (stage)
		{
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_VERTEX_BIT, "VERTEX");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TESSELLATION_CONTROL");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TESSELLATION_EVALUATION");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_GEOMETRY_BIT, "GEOMETRY");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_FRAGMENT_BIT, "FRAGMENT");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_COMPUTE_BIT, "COMPUTE");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TASK_BIT_EXT, "TASK");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MESH_BIT_EXT, "MESH");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "RAYGEN");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "ANY_HIT");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "CLOSEST_HIT");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MISS_BIT_KHR, "MISS");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, "INTERSECTION");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CALLABLE_BIT_KHR, "CALLABLE");
		}
		return res;
	}

	constexpr SlangStage ConvertToSlang(VkShaderStageFlagBits stage)
	{
		SlangStage res = {};
		switch (stage)
		{
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_VERTEX_BIT, SLANG_STAGE_VERTEX);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, SLANG_STAGE_HULL);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, SLANG_STAGE_DOMAIN);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_GEOMETRY_BIT, SLANG_STAGE_GEOMETRY);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_FRAGMENT_BIT, SLANG_STAGE_FRAGMENT);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_COMPUTE_BIT, SLANG_STAGE_COMPUTE);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TASK_BIT_EXT, SLANG_STAGE_AMPLIFICATION);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MESH_BIT_EXT, SLANG_STAGE_MESH);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_RAYGEN_BIT_KHR, SLANG_STAGE_RAY_GENERATION);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, SLANG_STAGE_ANY_HIT);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, SLANG_STAGE_CLOSEST_HIT);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MISS_BIT_KHR, SLANG_STAGE_MISS);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, SLANG_STAGE_INTERSECTION);
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CALLABLE_BIT_KHR, SLANG_STAGE_CALLABLE);
		}
		return res;
	}

	constexpr VkShaderStageFlagBits ConvertFromSlang(SlangStage stage)
	{
		VkShaderStageFlagBits res = {};
		switch (stage)
		{
			SWITCH_CASE_CONVERT2(SLANG_STAGE_VERTEX, VK_SHADER_STAGE_VERTEX_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_HULL, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_DOMAIN, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_GEOMETRY, VK_SHADER_STAGE_GEOMETRY_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_COMPUTE, VK_SHADER_STAGE_COMPUTE_BIT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_RAY_GENERATION, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_INTERSECTION, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_ANY_HIT, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_CLOSEST_HIT, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_MISS, VK_SHADER_STAGE_MISS_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_CALLABLE, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_MESH, VK_SHADER_STAGE_MESH_BIT_EXT);
			SWITCH_CASE_CONVERT2(SLANG_STAGE_AMPLIFICATION, VK_SHADER_STAGE_TASK_BIT_EXT);
		}
		return res;
	}


	std::string_view GetShadingLanguageName(ShadingLanguage l)
	{
		std::string_view res;
		switch (l)
		{
			SWITCH_CASE_CONVERT2(ShadingLanguage::Unknown, "Unknown");
			SWITCH_CASE_CONVERT2(ShadingLanguage::SPIR_V, "SPIR-V");
			SWITCH_CASE_CONVERT2(ShadingLanguage::GLSL, "GLSL");
			SWITCH_CASE_CONVERT2(ShadingLanguage::Slang, "SLang");
			SWITCH_CASE_CONVERT2(ShadingLanguage::HLSL, "HLSL");
		}
		return res;
	}

	bool ShaderInstance::compile(std::string const& code, std::string const& filename)
	{	
		const bool dump_source = application()->options().dump_shader_source;
		const bool dump_spv = application()->options().dump_shader_spv;
		if (_source_language == ShadingLanguage::GLSL)
		{
			shaderc::CompileOptions options;
			//options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_spirv_version_1_4);
			options.SetTargetSpirv(shaderc_spirv_version_1_6);
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			options.SetGenerateDebugInfo(); // To link the glsl source for nsight
			options.SetWarningsAsErrors();
			options.AddMacroDefinition("__glsl", "460");
			shaderc::Compiler compiler;

			shaderc::CompilationResult res = compiler.CompileGlslToSpv(code, getShaderKind(_stage), filename.c_str(), options);
			size_t errors = res.GetNumErrors();
			size_t warns = res.GetNumWarnings();

			if (errors)
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: "s,
					.error_message =
						"Error while compiling shader: \n"s +
						"Main Shader: "s + filename + ":\n"s +
						res.GetErrorMessage(),
				};
				return false;
			}
			_spv_code = std::vector<uint32_t>(res.cbegin(), res.cend());
		}
		else if(_source_language == ShadingLanguage::Slang)
		{
			Slang::ComPtr<slang::IGlobalSession> const& global_session = application()->getSlangSession();
			Slang::ComPtr<slang::ISession> session;
			slang::TargetDesc target{
				.format = SLANG_SPIRV,
				.profile = global_session->findProfile("glsl_460"),
			};
			slang::SessionDesc session_desc{
				.targets = &target,
				.targetCount = 1,
			};
			global_session->createSession(session_desc, session.writeRef());

			Slang::ComPtr<slang::ICompileRequest> compiler;
			session->createCompileRequest(compiler.writeRef());
			compiler->setCodeGenTarget(SLANG_SPIRV);
			compiler->setCompileFlags(SLANG_COMPILE_FLAG_NO_MANGLING);
			compiler->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_MAXIMAL);
			compiler->setDebugInfoFormat(SLANG_DEBUG_INFO_FORMAT_C7);
			compiler->setTargetEmbedDownstreamIR(0, true);
			int index = compiler->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, "0");
			compiler->addTranslationUnitSourceStringSpan(index, filename.c_str(), code.c_str(), code.c_str() + code.size());
			compiler->addPreprocessorDefine("__slang", "1");
			if (_stage != 0)
			{
				//compiler->addEntryPoint(index, "main", ConvertToSlang(_stage));
			}
			//compiler->setFileSystem

			SlangResult compilation_result = compiler->compile();
			Slang::ComPtr<slang::IBlob> diagnostic;
			if (SLANG_SUCCEEDED(compilation_result))
			{
				SlangResult sr;
				Slang::ComPtr<slang::IComponentType> slang_program;
				sr = compiler->getProgram(slang_program.writeRef());
				if (SLANG_SUCCEEDED(sr))
				{
					Slang::ComPtr<slang::IComponentType> linked;
					sr = slang_program->link(linked.writeRef(), diagnostic.writeRef());
					if (SLANG_SUCCEEDED(sr))
					{
						Slang::ComPtr<slang::IBlob> blob;
						sr = linked->getTargetCode(0, blob.writeRef(), diagnostic.writeRef());
						//sr = linked->getEntryPointCode(0, 0, blob.writeRef(), diagnostic.writeRef());
						if (SLANG_SUCCEEDED(sr))
						{
							const size_t size = blob->getBufferSize();
							_spv_code.resize(size / sizeof(uint32_t));
							std::memcpy(_spv_code.data(), blob->getBufferPointer(), size);
						}
					}

				}
			}
			if(_spv_code.empty())
			{
				std::string_view error = compiler->getDiagnosticOutput();
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: ",
					.error_message = 
						"Error while compiling shader: \n"s +
						"Main Shader: "s + filename + ":\n"s +
						std::string(error),
				};
				return false;
			}
		}
		else
		{
			_creation_result = AsynchTask::ReturnType{
				.success = false,
				.can_retry = true,
				.error_title = "Shader Compilation Error: "s,
				.error_message = std::format("Unsuported shading language: {}", GetShadingLanguageName(_source_language)),
			};
			return false;
		}

		if (dump_source || dump_spv)
		{
			// Path without mounting point
			that::ResultAnd<that::FileSystem::PathStringView> rel_path = that::FileSystem::ExtractRelative(_main_path);
			that::ResultAnd<that::FileSystem::PathStringView> mp = application()->fileSystem()->ExtractMountingPoint(_main_path);
			if (_source_language == ShadingLanguage::Slang)
			{
				VKL_BREAKPOINT_HANDLE;
			}
			if (rel_path.result == that::Result::Success && mp.result == that::Result::Success)
			{
				if (dump_source)
				{
					that::FileSystem::Path write_folder = "gen:/shaders_source/";
					that::FileSystem::Path write_path = write_folder / mp.value / rel_path.value;
					that::FileSystem::WriteFileInfo info = {};
					info.path = &write_path;
					info.data = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(code.data()), code.size());
					application()->fileSystem()->writeFile(info);
				}
				if (dump_spv)
				{
					that::FileSystem::Path spv_rel_path = rel_path.value;
					spv_rel_path += ".spv.bin";
					that::FileSystem::Path write_folder = "gen:/shaders_SPIRV-V/";
					that::FileSystem::Path write_path = write_folder / mp.value / spv_rel_path;
					that::FileSystem::WriteFileInfo info = {};
					info.path = &write_path;
					info.data = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(_spv_code.data()), _spv_code.byte_size());
					application()->fileSystem()->writeFile(info);
				}
			}
			else
			{
				application()->logger()(std::format("Could not extract dump path for shader {}!", _main_path.string()), Logger::Options::TagWarning);
			}
		}
		
		VkShaderModuleCreateInfo module_ci{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = _spv_code.size() * sizeof(uint32_t),
			.pCode = _spv_code.data(),
		};
		const VkResult result = vkCreateShaderModule(_app->device(), &module_ci, nullptr, &_module);
		if (result != VK_SUCCESS)
		{
			_creation_result = AsynchTask::ReturnType{
				.success = false,
				.can_retry = true,
				.error_title = "Shader Compilation Error: "s,
				.error_message =
					"Error while creating VkShaderModule: \n"s +
					"Main Shader: "s + _main_path.string() + ":\n"s
					"Error code: "s + std::to_string(result),
			};
			return false;
		}
		//VK_CHECK(result, "Failed to create a shader module.");

		return result == VK_SUCCESS;
	}

	void ShaderInstance::reflect()
	{
		spvReflectDestroyShaderModule(&_reflection);
		spvReflectCreateShaderModule(_spv_code.size() * sizeof(uint32_t), _spv_code.data(), &_reflection);
	}

	ShaderInstance::ShaderInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_main_path(ci.source_path),
		_source_language(ci.source_language),
		_stage(ci.stage),
		_reflection(std::zeroInit(_reflection)),
		_shader_string_packed_capacity(ci.shader_string_packed_capacity)
	{
		_creation_result.success = true;
		using namespace std::containers_append_operators;
		std::filesystem::file_time_type compile_time = std::chrono::file_clock::now();

		std::string semantic_definition = std::format("SHADER_SEMANTIC_{} 1", getShaderStageName(_stage));
		DefinitionsList defines = { semantic_definition };
		defines += application()->commonShaderDefinitions().collapsed();
		defines += ci.definitions;

		_preprocessed_source = preprocess(ci.source_path, defines);
		
		if (_creation_result.success)
		{
			compile_time = std::chrono::file_clock::now();
			compile(_preprocessed_source, ci.source_path.string());

			if (_creation_result.success)
			{
				compile_time = std::chrono::file_clock::now();
				reflect();
			}
		}

		
		if (_creation_result.success == false && _creation_result.can_retry)
		{
			_creation_result.auto_retry_f = [this, compile_time]() -> bool
			{
				const std::filesystem::file_time_type update_time = [&]() {
					that::FileSystem & fs = *application()->fileSystem();
					std::filesystem::file_time_type res = std::filesystem::file_time_type::min();
					for (const auto& dep : _dependencies)
					{
						that::ResultAnd<that::FileSystem::TimePoint> file_time = fs.getFileLastWriteTime(dep, that::FileSystem::Hint::PathIsNative);
						if (file_time.result == that::Result::Success)
						{
							res = std::max(res, file_time.value);
						}
					}
					return res;
				}();
				return update_time > compile_time;
			};
		}
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
			.pNext = nullptr,
			.flags = 0,
			.stage = _stage,
			.module = module(),
			.pName = _reflection.entry_point_name,
			.pSpecializationInfo = nullptr,
		};
	}

	void Shader::createInstance(SpecializationKey const& key, DefinitionsList const& common_definitions, size_t string_packed_capacity)
	{
		waitForInstanceCreationIFN();
		if (_specializations.contains(key))
		{
			_inst = _specializations[key];
		}
		else {

			using namespace std::containers_append_operators;
			DefinitionsList definitions;
			if(_definitions)
				definitions = (*_definitions);
			definitions += common_definitions;

			SpecializationKey lkey = key;
			
			_create_instance_task = std::make_shared<AsynchTask>(AsynchTask::CI{
				.name = "Compiling shader " + _path.string(),
				.verbosity = AsynchTask::Verbosity::Medium,
				.priority = TaskPriority::ASAP(),
				.lambda = [this, definitions = std::move(definitions), string_packed_capacity, lkey]() {

					_inst = std::make_shared<ShaderInstance>(ShaderInstance::CI{
						.app = application(),
						.name = name(),
						.source_path = _path,
						.stage = _stage,
						.definitions = definitions,
						.shader_string_packed_capacity = string_packed_capacity,
					});

					const AsynchTask::ReturnType & res = _inst->getCreationResult();
					
					if (res.success)
					{
						_specializations[lkey] = _inst;
						_instance_time = std::chrono::file_clock::now();
						_dependencies = _inst->dependencies();
					}

					return res;
				},
			});
			application()->threadPool().pushTask(_create_instance_task);
		}
	}

	void Shader::destroyInstanceIFN()
	{
		waitForInstanceCreationIFN();
		ParentType::destroyInstanceIFN();
	}

	bool Shader::updateResources(UpdateContext & ctx)
	{
		using namespace std::containers_append_operators;

		static thread_local SpecializationKey new_key;
		
		if (ctx.updateTick() <= _update_tick)
		{
			return _latest_update_result;
		}
		_update_tick = ctx.updateTick();
		bool &res = _latest_update_result = false;

		if (checkHoldInstance())
		{
			if (ctx.checkShadersTick() > _check_tick)
			{
				waitForInstanceCreationIFN();
				for (const auto& dep : _dependencies)
				{
					const that::ResultAnd<std::filesystem::file_time_type> new_time = application()->fileSystem()->getFileLastWriteTime(dep);
					if (new_time.result == that::Result::Success && new_time.value > _instance_time)
					{
						_specializations.clear();
						res = true;
						break;
					}
				}
				_check_tick = ctx.checkShadersTick();
			}

			if (_definitions.hasValue())
			{
				new_key.clear();

				DefinitionsList definitions = *_definitions;
				definitions += ctx.commonDefinitions()->collapsed();

				for (size_t i = 0; i < definitions.size(); ++i)
				{
					new_key.definitions += definitions[i];
					new_key.definitions += '\n';
				}
				const bool use_different_spec = new_key != _current_key;
				if (use_different_spec)
				{
					_current_key = new_key;
					res = true;
				}
			}

			if (res)
			{
				destroyInstanceIFN();
			}
		
		
			if (!_inst)
			{
				std::string capacity = ctx.commonDefinitions()->getDefinition("SHADER_STRING_CAPACITY");
				uint32_t packed_capcity = 32;
				if(!capacity.empty())
				{	
					// TODO use a better function that checks the result and can parse hex
					packed_capcity = std::atoi(capacity.c_str());
				}
				createInstance(_current_key, ctx.commonDefinitions()->collapsed(), static_cast<size_t>(packed_capcity));
				res = true;
			}
		}

		return res;
	}

	Shader::Shader(CreateInfo const& ci):
		ParentType(ci.app, ci.name, ci.hold_instance),
		_path(ci.source_path),
		_stage(ci.stage),
		_definitions(ci.definitions)
	{
		_instance_time = std::filesystem::file_time_type::min();
	}

	Shader::~Shader()
	{
		// TODO can cancel task
		destroyInstanceIFN();
	}

	void Shader::waitForInstanceCreationIFN()
	{
		if (_create_instance_task)
		{
			_create_instance_task->waitIFN();
			assert(_create_instance_task->isSuccess());
			_create_instance_task = nullptr;
		}
	}

	std::shared_ptr<ShaderInstance> Shader::getInstanceWaitIFN()
	{
		waitForInstanceCreationIFN();
		return instance();
	}
}