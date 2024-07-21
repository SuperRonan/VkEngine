include(CMakeParseArguments)

set(PROJECTS_FOLDER "${CMAKE_CURRENT_LIST_DIR}/")

set(EXECUTABLES "")

function(AddLib target)
	cmake_parse_arguments(PARSED_ARGS "" "SUB_DIRECTORY" "" ${ARGN})
	if(NOT PARSED_ARGS_SUB_DIRECTORY)
		set(PARSED_ARGS_SUB_DIRECTORY ${target})
	endif()
	set(folder ${PARSED_ARGS_SUB_DIRECTORY})
	file(GLOB_RECURSE ${target}_SOURCES ${folder}/*)
	add_library(${target} ${${target}_SOURCES})
	
	source_group(TREE ${PROJECTS_FOLDER}${folder} FILES ${${target}_SOURCES})

	target_include_directories(${target} PUBLIC  ${folder})

	target_link_libraries(${target} PUBLIC VkEngine)

	set_target_properties(${target} PROPERTIES LINK_OPTIONS ${VK_ENGINE_LINK_OPTIONS})

	set_target_properties(${target} PROPERTIES LINK_FLAGS "/ignore:4099") # PDB of provided libraries not found, TODO don't have the warning in the first place

endfunction()

function(AddExec target)
	cmake_parse_arguments(PARSED_ARGS "" "SUB_DIRECTORY;MAIN_SOURCE" "DEPENDENCIES" ${ARGN})
	if(NOT PARSED_ARGS_SUB_DIRECTORY)
		set(PARSED_ARGS_SUB_DIRECTORY ${target})
	endif()
	set(folder ${PARSED_ARGS_SUB_DIRECTORY})
	file(GLOB_RECURSE ${target}_SOURCES ${folder}/*)
	add_executable(${target} ${folder}/main.cpp ${${target}_SOURCES})
	
	source_group(TREE ${PROJECTS_FOLDER}${folder} FILES ${${target}_SOURCES})
	
	target_include_directories(${target} PRIVATE ${folder})
	
	target_compile_definitions(${target} PRIVATE "PROJECT_NAME=\"${target}\"")

	set(${folder}_DEPENDENCIES VkEngine)
	if(PARSED_ARGS_DEPENDENCIES)
		list(APPEND ${folder}_DEPENDENCIES ${PARSED_ARGS_DEPENDENCIES})
	endif()

	target_link_libraries(${target} PUBLIC ${${folder}_DEPENDENCIES})

	

	set_target_properties(${target} PROPERTIES LINK_OPTIONS ${VK_ENGINE_LINK_OPTIONS})
	
	set_target_properties(${target} PROPERTIES LINK_FLAGS "/ignore:4099") # PDB of provided libraries not found, TODO don't have the warning in the first place
	set(EXECUTABLES ${EXECUTABLES} ${target})
	
endfunction()

AddLib(RenderLib)

AddExec(Test)

# AddExec(GameOfLife "GOL")

# AddExec(Paint)

# AddExec(Particules2D "Particules/sim2D")
# AddExec(Particules3D "Particules/sim3D")

# AddExec(Flotsam)

AddExec(Renderer DEPENDENCIES RenderLib)
AddExec(BSDF DEPENDENCIES RenderLib)

set(MP_CONTENT "\"ShaderLib\" \"${VKL_SHADER_FOLDER}/\"")
set(MP_CONTENT "${MP_CONTENT}\n\"gen\" \"${ENGINE_SRC_PATH}/../gen/\"")
set(MP_CONTENT "${MP_CONTENT}\n\"DevProjectsFolder\" \"${PROJECTS_FOLDER}\"")
set(MP_CONTENT "${MP_CONTENT}\n\"RenderLibShaders\" \"${PROJECTS_FOLDER}RenderLib/Shaders/\"")
file(GENERATE OUTPUT "$<CONFIG>/MountingPoints.txt" CONTENT ${MP_CONTENT})

install(TARGETS ${EXECUTABLES} DESTINATION bin)

# message(${LINK_FLAGS_DEBUG})