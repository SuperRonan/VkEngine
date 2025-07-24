#include <vkl/VkObjects/Shader.hpp>

#include <iostream>
#include <fstream>
#include <exception>
#include <cassert>
#include <sstream>
#include <string_view>
#include <format>
#include <thread>
#include <set>

#include <slang/slang.h>
#include <slang/slang-gfx.h>

#define SWITCH_CASE_CONVERT(CASE, VALUE, target) \
case CASE: \
	target = VALUE; \
break;

#define SWITCH_CASE_CONVERT2(CASE, VALUE) SWITCH_CASE_CONVERT(CASE, VALUE, res)

namespace vkl
{
	that::FileSystem::Path ShaderInstance::resolveIncludePath(that::FileSystem::Path const& path, PreprocessingState& preprocessing_state, IncludeType include_type)
	{
		that::FileSystem::Path res;
		that::FileSystem& fs = *application()->fileSystem();
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
					path.string(), that::GetResultStr(resolved_path.result), static_cast<std::underlying_type<that::Result>::type>(resolved_path.result), _main_path.string()
				),
			};
			return {};
		}
		res = resolved_path.value;
		const auto validate_path = [&]() -> bool
		{
			return (fs.checkFileExists(res, that::FileSystem::Hint::PathIsNative) == that::Result::Success);
		};
		bool path_is_valid = false;
		res = resolved_path.value;
		path_is_valid = validate_path();
		if (include_type == IncludeType::Brackets)
		{
			if (!path_is_valid)
			{
				for (const auto& include_dir : preprocessing_state.include_directories)
				{
					res = include_dir / resolved_path.value;
					path_is_valid = validate_path();
					if (path_is_valid)
					{
						break;
					}
				}
			}
		}

		if (!path_is_valid)
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
			res.clear();
		}

		if (!res.empty())
		{
			 auto cnn = fs.cannonize(res);
			 if (cnn.result == that::Result::Success)
			 {
				res = cnn.value;
			 }
			 else
			 {
				 _creation_result = AsynchTask::ReturnType{
				 .success = false,
				 .can_retry = true,
				 .error_title = "Shader Compilation Error: Invalid file path"s,
				 .error_message = std::format(
					 "Error while preprocessing shader : Could not cannonize path : {} \n"
					 "Original path: {}\n"
					 "Resolved path: {}\n"
					 "Main Shader: {}\n"
					 "Error: {} (code: {})\n",
					 res.string(), path.string(), resolved_path.value.string(), _main_path.string(), that::GetResultStrSafe(cnn.result), static_cast<std::underlying_type<that::Result>::type>(cnn.result)
				 ),
				 };
				 res.clear();
			 }
		}
		
		return res;
	}

	ShaderInstance::PreprocessResult ShaderInstance::includeFile(that::FileSystem::Path const& path, PreprocessingState& preprocessing_state, size_t recursion_level, IncludeType include_type, that::FileSystem::Path * resolved_path)
	{
		that::FileSystem::Path _full_path;
		that::FileSystem::Path & full_path = resolved_path ? *resolved_path : _full_path;
		full_path = resolveIncludePath(path, preprocessing_state, include_type);
		PreprocessResult res;
		if (!full_path.empty())
		{
			_dependencies.push_back(full_path);

			if ((include_type != IncludeType::None) && preprocessing_state.pragma_once_files.contains(full_path))
			{
				_creation_result.success = true;
				return { std::string(), 1 };
			}
			else
			{
				that::Result read_result = application()->fileSystem()->readFile(that::FileSystem::ReadFileInfo{
					.hint = that::FileSystem::Hint::PathIsNative | that::FileSystem::Hint::PathIsCannon,
					.path = &full_path,
					.result_string = &res.content,
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
					return {};
				}
			}
		}
		else
		{
			return {};
		}
		return res;
	}

	bool ShaderInstance::checkPragmaOnce(std::string& content, size_t copied_so_far) const
	{
		const size_t pragma_once_begin = content.find("#pragma once", copied_so_far); // TODO regex?
		bool res = false;
		// TODO ckeck it is not in a commented section
		if (pragma_once_begin != std::string::npos)
		{
			// Comment the #pragma once so the glsl compiler doesn't print a warning
			content[pragma_once_begin] = '/';
			content[pragma_once_begin + 1] = '/';
			res = true;
		}
		return res;
	}

	ShaderInstance::PreprocessResult ShaderInstance::preprocessIncludesAndDefinitions(that::FileSystem::Path const& path, PreprocessingState & preprocessing_state, size_t recursion_level, IncludeType include_type)
	{
		that::FileSystem::Path full_path;
		PreprocessResult included = includeFile(path, preprocessing_state, recursion_level, include_type, &full_path);
		std::string & content = included.content;
		if (!_creation_result.success || included.flags & 1)
		{
			return included;
		}
		
		std::stringstream oss;

		size_t copied_so_far = 0;

		const auto countLines = [&](size_t b, size_t e)
		{
			return std::count(content.data() + b, content.data() + e, '\n');
		};

		if (!preprocessing_state.definitions.empty())
		{
			DefinitionsList & definitions = preprocessing_state.definitions;
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
					return {};
				}
				const size_t version_end = content.find("\n", version_begin);

				oss << std::string_view(content.data() + copied_so_far, version_end - copied_so_far);
				copied_so_far = version_end;
			}
			for (size_t i = 0; i < definitions.size(); ++i)
			{
				oss << "#define " << definitions[i] << "\n";
			}
			oss << "#line " << (countLines(0, copied_so_far) + 1) << ' ' << full_path << "\n";
			definitions.clear();
		}


		if (checkPragmaOnce(content, copied_so_far))
		{
			preprocessing_state.pragma_once_files.emplace(full_path);
		}


		while (true)
		{

			// TODO check if not in comment
			const size_t include_begin = content.find("#include", copied_so_far);
				
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
						const that::FileSystem::Path folder = full_path.parent_path();
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
				
				{
					const auto [included_code, include_flags] = preprocessIncludesAndDefinitions(path_to_include, preprocessing_state, recursion_level + 1, include_type);
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
						if (include_flags & 1)
						{
							//oss << "//" << include_line << " : Pruned by #pragma once.\n";

						}
						else
						{
							oss << "#line 1 " << path_to_include << "\n";
							oss << included_code;
							oss << "\n#line " << (countLines(0, line_end) + 1) << ' ' << full_path << "\n";
						}
					}
				}
				copied_so_far = line_end;

			}
			else
				break;
		}

		oss << std::string_view(content.data() + copied_so_far, content.size() - copied_so_far);

		return {oss.str(), 0};
	}
	
	std::string ShaderInstance::preprocessStrings(const std::string& glsl, bool ignore_includes)
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
			if (!ignore_includes)
			{
				const size_t include_line_begin = line.find("#include");
				if (include_line_begin != std::string::npos)
				{
					return true;
				}
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
	
	std::string ShaderInstance::preprocess(that::FileSystem::Path const& path, PreprocessingState & preprocessing_state)
	{
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

		if (_source_language == ShadingLanguage::Slang)
		{
			preprocessing_state.full_manual_preprocess = true;
		}

		if (preprocessing_state.full_manual_preprocess)
		{
			try
			{
				full_source = preprocessIncludesAndDefinitions(path, preprocessing_state, 0, IncludeType::None).content;
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
		}
		else
		{
			PreprocessResult included = includeFile(path, preprocessing_state, 0, IncludeType::None);
			full_source = std::move(included.content);
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

	constexpr const char* GetGLSLShaderExtension(VkShaderStageFlagBits stage)
	{
		const char * res = nullptr;
		switch (stage)
		{
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_VERTEX_BIT, "vert");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "tesc");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "tese");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_GEOMETRY_BIT, "geom");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_FRAGMENT_BIT, "frag");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_COMPUTE_BIT, "comp");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_TASK_BIT_EXT, "task");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MESH_BIT_EXT, "mesh");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "rgen");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "rahit");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "rchit");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_MISS_BIT_KHR, "rmiss");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, "rint");
			SWITCH_CASE_CONVERT2(VK_SHADER_STAGE_CALLABLE_BIT_KHR, "rcall");
		}
		if (!res)
		{
			res = "glsl";
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

	// Given something like:
	// "IDENTIFIER VALUE" -> {"IDENTIFIER", "VALUE"}
	// "IDENTIFIER" -> {"IDENTIFIER", {}}
	// The input is not expected to be of the form: "#define IDENTIFIER VALUE"
	std::pair<std::string_view, std::string_view> ParseDefinition(std::string_view def)
	{
		using namespace std::string_literals;
		std::pair<std::string_view, std::string_view> res = {};
		const std::string_view whitespace = " \t"sv;
		const size_t it0 = def.find_first_not_of(whitespace);
		const size_t it = def.find_first_of(whitespace, it0);
		if (it != std::string_view::npos)
		{
			res.first = std::string_view(def.data(), it);

			const size_t it2 = def.find_first_not_of(whitespace, it);
			if (it2 != std::string_view::npos)
			{
				res.second = std::string_view(def.data() + it2, def.size() - it2);
			}
		}
		return res;
	}


	struct ShaderCIncluderInterface : public::shaderc::CompileOptions::IncluderInterface
	{
		ShaderInstance * that = nullptr;
		ShaderInstance::PreprocessingState * preprocessing_state;

		struct Result
		{
			// Don't use std::string because we can't allow short string optimization 
			MyVector<char> name = {};
			MyVector<char> content = {};
		};

		// Both the same size
		MyVector<std::unique_ptr<shaderc_include_result>> _produced_results = {};
		MyVector<Result> _results = {};
		
		virtual ~ShaderCIncluderInterface()
		{}

		virtual shaderc_include_result* GetInclude(const char* requested_source, const shaderc_include_type type, const char* requesting_source, size_t include_depth) final override
		{
			that::FileSystem::Path path = requested_source;
			that::FileSystem::Path resolved_path;
			ShaderInstance::IncludeType include_type = {};
			switch (type)
			{
				SWITCH_CASE_CONVERT(shaderc_include_type_relative, ShaderInstance::IncludeType::Quotes, include_type);
				SWITCH_CASE_CONVERT(shaderc_include_type_standard, ShaderInstance::IncludeType::Brackets, include_type);
			}
			if (include_type == ShaderInstance::IncludeType::Quotes)
			{
				that::FileSystem::Path folder = that::FileSystem::Path(requesting_source);
				//folder = that->application()->fileSystem()->resolve(folder).value;
				folder = folder.parent_path();
				path = folder / path;
			}

			ShaderInstance::PreprocessResult included = that->includeFile(path, *preprocessing_state, include_depth, include_type, &resolved_path);
			bool success = that->_creation_result.success;
			
			std::unique_ptr<shaderc_include_result> res = std::make_unique<shaderc_include_result>();
			shaderc_include_result * res_ptr = res.get();
			size_t index = _produced_results.size();
			reinterpret_cast<size_t&>(res->user_data) = index;

			_produced_results.emplace_back(std::move(res));
			_results.push_back({});
			Result & storage = _results.back();

			
			if (success)
			{
				std::string tmp = resolved_path.string();
				storage.name = MyVector<char>(tmp.begin(), tmp.end());
				std::string & code = included.content;

				// shaderc does not handle #pragma once itself
				if (that->checkPragmaOnce(code))
				{
					preprocessing_state->pragma_once_files.insert(resolved_path);
				}

				code = that->preprocessStrings(code);
				storage.content = MyVector<char>(code.begin(), code.end());
			}
			else
			{
				storage.name = {};
				storage.content = MyVector<char>(that->_creation_result.error_message.begin(), that->_creation_result.error_message.end());
			}

			res_ptr->source_name = storage.name.data();
			res_ptr->source_name_length = storage.name.size();
			res_ptr->content = storage.content.data();
			res_ptr->content_length = storage.content.size();

			return res_ptr;
		}

		virtual void ReleaseInclude(shaderc_include_result* data) final override
		{
			size_t index = reinterpret_cast<size_t>(data->user_data);
			if (index < _produced_results.size())
			{
				_produced_results[index].reset();
				_results[index] = {};
			}
		}
	};

	struct SlangFileSystemInterface : public ISlangFileSystem
	{
		ShaderInstance* that = nullptr;

		virtual SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(char const* path, ISlangBlob** outBlob)
		{

		}
	};

	bool ShaderInstance::compile(std::string const& pcode, PreprocessingState & preprocessing_state)
	{	
		const bool dump_source = application()->options().dump_shader_source;
		const bool dump_preprocessed = application()->options().dump_shader_preprocessed;
		const bool dump_spv = application()->options().dump_shader_spv;
		const bool dump_slang_to_glsl = application()->options().dump_slang_to_glsl;
		const std::string main_path_str = _main_path.string();

		std::string const * code = &pcode;
		std::string preprocessed_tmp;
		std::string const* preprocessed_code = nullptr;

		// TODO the dumped code is copied from already existing memory
		// TODO avoid these copies;

		std::string slang_to_glsl;

		if (_source_language == ShadingLanguage::Slang)
		{
			Slang::ComPtr<slang::IGlobalSession> global_session = application()->getSlangGlobalSession();
			Slang::ComPtr<slang::ISession> session;
			std::array<slang::TargetDesc, 2> targets = {
				slang::TargetDesc{
					.format = SLANG_SPIRV,
					//.profile = global_session->findProfile("spirv_1_6"), // may generate spv-val errors :(
					.profile = global_session->findProfile("glsl_460"), // doesn't generate spv-val errors :)
					.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY,
				},
				slang::TargetDesc{
					.format = SLANG_GLSL,
					.profile = global_session->findProfile("glsl_460"),
				},
			};
			uint num_targets = 1;
			if (dump_slang_to_glsl)
			{
				num_targets++;
			}
			const SlangOptimizationLevel opt_level = std::clamp(static_cast<SlangOptimizationLevel>(application()->options().slang_optiomization_level), SLANG_OPTIMIZATION_LEVEL_NONE, SLANG_OPTIMIZATION_LEVEL_MAXIMAL);

			MyVector<slang::CompilerOptionEntry> compiler_options{
				slang::CompilerOptionEntry{
					.name = slang::CompilerOptionName::Optimization,
					.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = static_cast<int>(opt_level)},
				},
				slang::CompilerOptionEntry{
					.name = slang::CompilerOptionName::NoMangle,
					.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1},
				},
				slang::CompilerOptionEntry{
					.name = slang::CompilerOptionName::MacroDefine,
					.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::String, .stringValue0 = "_slang", .stringValue1 = "1"},
				},
				//slang::CompilerOptionEntry{
				//	.name = slang::CompilerOptionName::DumpIr,
				//	.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1},
				//},
			};

			if (_generate_debug_info)
			{
				compiler_options.push_back(slang::CompilerOptionEntry{
					.name = slang::CompilerOptionName::DebugInformation,
					.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL},
				});
				compiler_options.push_back(slang::CompilerOptionEntry{
					.name = slang::CompilerOptionName::DebugInformationFormat,
					.value = slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = SLANG_DEBUG_INFO_FORMAT_C7},
				});
				_has_debug_info = true;
			}
			

			SlangResult sr;
			Slang::ComPtr<slang::IBlob> diagnostic;

			slang::SessionDesc session_desc{
				.targets = targets.data(),
				.targetCount = num_targets,
				.flags = SLANG_COMPILE_FLAG_NO_MANGLING,
				.compilerOptionEntries = compiler_options.data(),
				.compilerOptionEntryCount = compiler_options.size32(),
			};

			if (_stage != 0)
			{
				//compiler->addEntryPoint(index, "main", ConvertToSlang(_stage));
			}
			//compiler->setFileSystem

			try
			{
				sr = global_session->createSession(session_desc, session.writeRef());
				if (SLANG_SUCCEEDED(sr))
				{
					auto module = session->loadModuleFromSourceString("MyShader", main_path_str.c_str(), code->c_str(), diagnostic.writeRef());
					if (!!module)
					{
						Slang::ComPtr<slang::IEntryPoint> ep;
						sr = module->findAndCheckEntryPoint("main", ConvertToSlang(_stage), ep.writeRef(), diagnostic.writeRef());
						if (SLANG_SUCCEEDED(sr))
						{
							Slang::ComPtr<slang::IComponentType> linked;
							sr = ep->linkWithOptions(linked.writeRef(), 0, nullptr, diagnostic.writeRef());
							if (SLANG_SUCCEEDED(sr))
							{
								auto get_target_code = [&](SlangInt32 target, auto & result_buffer)
								{
									SlangResult sr = {};
									Slang::ComPtr<slang::IBlob> blob;
									sr = linked->getTargetCode(target, blob.writeRef(), diagnostic.writeRef());
									if (SLANG_SUCCEEDED(sr))
									{
										using Result_t = typename std::remove_cvref<decltype(result_buffer)>::type;
										constexpr const size_t ElemSize = sizeof(typename Result_t::value_type);
										const size_t size = blob->getBufferSize();
										result_buffer.resize((size + ElemSize - 1) / ElemSize);
										std::memcpy(result_buffer.data(), blob->getBufferPointer(), size);
									}
									return sr;
								};

								get_target_code(0, _spv_code);
								if (dump_slang_to_glsl)
								{
									get_target_code(1, slang_to_glsl);
								}
							}
						}
					}
				}
			}
			catch (std::exception const& e)
			{
				std::string_view error = e.what();
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Exception Caught: ",
					.error_message = std::format(
						"Exception while compiling shader: \n"
						"Main Shader: {}:\n"
						"{}",
						main_path_str, error
					),
				};
				return false;
			}
			catch (...)
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Exception Caught: ",
					.error_message = std::format(
						"Unknown Exception while compiling shader: \n"
						"Main Shader: {}:\n",
						main_path_str
					),
				};
				return false;
			}
			if (_spv_code.empty())
			{
				std::string_view error(static_cast<const char*>(diagnostic->getBufferPointer()), diagnostic->getBufferSize());
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: ",
					.error_message = std::format(
						"Error while compiling shader: \n"
						"Main Shader: {}:\n"
						"{}",
						main_path_str, error
					),
				};
				return false;
			}
		}
		if (_source_language == ShadingLanguage::GLSL)
		{
			shaderc::CompileOptions options;
			//options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_spirv_version_1_4);
			options.SetTargetSpirv(shaderc_spirv_version_1_6);
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			if (_generate_debug_info || true)
			{
				// We need the debug info for the reflection of the bindings names
				options.SetGenerateDebugInfo(); // To link the glsl source for nsight
				_has_debug_info = true;
			}
			options.SetWarningsAsErrors();
			options.AddMacroDefinition("_glsl", "460");
			shaderc_optimization_level opt_level = std::clamp(static_cast<shaderc_optimization_level>(application()->options().shaderc_optimization_level), shaderc_optimization_level_zero, shaderc_optimization_level_performance);
			options.SetOptimizationLevel(opt_level);

			{
				DefinitionsList & definitions = preprocessing_state.definitions;
				for (size_t i = 0; i < definitions.size(); ++i)
				{
					std::string_view def = definitions.at(i);
					auto [id, value] = ParseDefinition(def);
					if (id.empty())
					{
						VKL_BREAKPOINT_HANDLE;
					}
					else
					{
						options.AddMacroDefinition(id.data(), id.size(), value.data(), value.size());
					}
				}
				definitions.clear();
			}
			
			std::unique_ptr<ShaderCIncluderInterface> includer = std::make_unique<ShaderCIncluderInterface>();
			includer->that = this;
			includer->preprocessing_state = &preprocessing_state;
			options.SetIncluder(std::move(includer));

			shaderc::Compiler compiler;

			if (dump_preprocessed)
			{
				shaderc::CompilationResult preprocessed_res = compiler.PreprocessGlsl(*code, getShaderKind(_stage), main_path_str.c_str(), options);
				preprocessed_tmp = std::string(preprocessed_res.cbegin(), preprocessed_res.cend());
				preprocessed_code = &preprocessed_tmp;
				size_t errors = preprocessed_res.GetNumErrors();
				size_t warns = preprocessed_res.GetNumWarnings();
				if (errors)
				{
					_creation_result = AsynchTask::ReturnType{
						.success = false,
						.can_retry = true,
						.error_title = "Shader Compilation Error: "s,
						.error_message = std::format(
							"Error while preprocessing shader: \n"
							"Main Shader: {}:\n"
							"{}",
							_main_path.string(), preprocessed_res.GetErrorMessage()
						),
					};
					return false;
				}
			}

			if (preprocessed_code)
			{
				code = preprocessed_code;
			}

			shaderc::CompilationResult compile_res = compiler.CompileGlslToSpv(*code, getShaderKind(_stage), main_path_str.c_str(), options);
			size_t errors = compile_res.GetNumErrors();
			size_t warns = compile_res.GetNumWarnings();

			if (errors)
			{
				_creation_result = AsynchTask::ReturnType{
					.success = false,
					.can_retry = true,
					.error_title = "Shader Compilation Error: "s,
					.error_message = std::format(
						"Error while compiling shader: \n"
						"Main Shader: {}:\n"
						"{}",
						_main_path.string(), compile_res.GetErrorMessage()
					),
				};
				return false;
			}
			_spv_code = std::vector<uint32_t>(compile_res.cbegin(), compile_res.cend());
		}
		else if(_creation_result.success && _spv_code.empty())
		{
			_creation_result = AsynchTask::ReturnType{
				.success = false,
				.can_retry = true,
				.error_title = "Shader Compilation Error: "s,
				.error_message = std::format("Unsuported shading language: {}", GetShadingLanguageName(_source_language)),
			};
			return false;
		}

		if (dump_source || dump_preprocessed || dump_spv || dump_slang_to_glsl)
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
					info.data = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(code->data()), code->size());
					application()->fileSystem()->writeFile(info);
				}
				if (dump_preprocessed && preprocessed_code)
				{
					that::FileSystem::Path write_folder = "gen:/preprocessed_shaders/";
					that::FileSystem::Path write_path = write_folder / mp.value / rel_path.value;
					that::FileSystem::WriteFileInfo info = {};
					info.path = &write_path;
					info.data = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(preprocessed_code->data()), preprocessed_code->size());
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
				if (dump_slang_to_glsl && _source_language == ShadingLanguage::Slang)
				{
					if (!slang_to_glsl.empty())
					{
						that::FileSystem::Path write_folder = "gen:/shaders_GLSL_from_Slang/";
						that::FileSystem::Path write_path = write_folder / mp.value / rel_path.value;
						const char * glsl_ext = GetGLSLShaderExtension(_stage);
						write_path.replace_extension(glsl_ext);
						that::FileSystem::WriteFileInfo info = {};
						info.path = &write_path;
						info.data = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(slang_to_glsl.data()), slang_to_glsl.size());
						application()->fileSystem()->writeFile(info);
					}
					else
					{
						application()->logger().log(std::format("Failed to generate GLSL code from Slang shader: {}", _main_path.string()), Logger::Options::TagWarning);
					}
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
		_shader_string_packed_capacity(ci.shader_string_packed_capacity),
		_generate_debug_info(ci.generate_debug_info)
	{
		_creation_result.success = true;
		using namespace std::containers_append_operators;
		std::filesystem::file_time_type compile_time = std::chrono::file_clock::now();

		PreprocessingState preproc;
		auto addDefinitions = [&]()
		{
			DefinitionsList& defines = preproc.definitions;
			defines.pushBack(std::format("SHADER_SEMANTIC_{} 1", getShaderStageName(_stage)));
			defines.pushBack("_VKL 1");
			defines += application()->commonShaderDefinitions().collapsed();
			defines += ci.definitions;
		};

		preproc.include_directories = application()->includeDirectories();
		addDefinitions();
		preproc.full_manual_preprocess = _source_language == ShadingLanguage::Slang;

		_preprocessed_source = preprocess(ci.source_path, preproc);
		
		if (_creation_result.success)
		{
			compile_time = std::chrono::file_clock::now();
			compile(_preprocessed_source, preproc);

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

	void Shader::createInstance(SpecializationKey const& key, DefinitionsList const& common_definitions, size_t string_packed_capacity, bool generate_shader_debug_info)
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
				.lambda = [this, definitions = std::move(definitions), string_packed_capacity, lkey, generate_shader_debug_info]() {

					_inst = std::make_shared<ShaderInstance>(ShaderInstance::CI{
						.app = application(),
						.name = name(),
						.source_path = _path,
						.stage = _stage,
						.definitions = definitions,
						.shader_string_packed_capacity = string_packed_capacity,
						.generate_debug_info = generate_shader_debug_info,
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
				createInstance(_current_key, ctx.commonDefinitions()->collapsed(), static_cast<size_t>(packed_capcity), application()->options().generate_shader_debug_info);
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