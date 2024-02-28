set(EXECUTABLES "")

function(AddProject p_name)
	if(DEFINED ARGV1)
		set(folder ${ARGV1})
	else()
		set(folder ${p_name})
	endif()
	file(GLOB ${p_name}_SOURCES ${folder}/*)
	add_executable(${p_name} ${folder}/main.cpp ${${p_name}_SOURCES})
	target_include_directories(${p_name} PRIVATE ${folder})
	
	target_compile_definitions(${p_name} PRIVATE "PROJECT_NAME=\"${p_name}\"")
	target_compile_definitions(${p_name} PRIVATE "PROJECT_SRC_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/${folder}/\"")

	target_link_libraries(${p_name} PUBLIC VkEngine)
	
	set_target_properties(${p_name} PROPERTIES LINK_FLAGS "/ignore:4099") # PDB of provided libraries not found, TODO don't have the warning in the first place
	set(EXECUTABLES ${EXECUTABLES} ${p_name})

	
endfunction()


AddProject(Test)

# AddProject(GameOfLife "GOL")

# AddProject(Paint)

# AddProject(Particules2D "Particules/sim2D")
# AddProject(Particules3D "Particules/sim3D")

# AddProject(Flotsam)

AddProject(Renderer)

set(MP_CONTENT "\"ShaderLib\" \"${ENGINE_SRC_PATH}/Shaders/\"")
set(MP_CONTENT "${MP_CONTENT}\n\"gen\" \"${ENGINE_SRC_PATH}/../gen/\"")
set(MP_CONTENT "${MP_CONTENT}\n\"DevProjectsFolder\" \"${ENGINE_SRC_PATH}/../projects/\"")
file(GENERATE OUTPUT "$<CONFIG>/MountingPoints.txt" CONTENT ${MP_CONTENT})

install(TARGETS ${EXECUTABLES} DESTINATION bin)