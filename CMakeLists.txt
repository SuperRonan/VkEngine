
CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0)

SET(Project_Name "VkEngine")
PROJECT(${Project_Name})

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# SET(CMAKE_CXX_FLAGS_DEBUG	"/MD")
# SET(CMAKE_CXX_FLAGS_RELEASE	"/MD")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# ThatLib
ADD_SUBDIRECTORY(ext/thatlib)
SET(THAT_LIB "thatlib")

# GLFW
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
ADD_SUBDIRECTORY(ext/glfw)
INCLUDE_DIRECTORIES(ext/glfw/include)
SET(GLFW_LIBS "glfw")

# GLM
ADD_SUBDIRECTORY(ext/glm)
INCLUDE_DIRECTORIES(ext/glm)
SET(GLM_LIBS "")

# Vulkan
OPTION(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON)

IF(AUTO_LOCATE_VULKAN)
	MESSAGE(STATUS "Attempting auto locate Vulkan using CMake...")

	FIND_PACKAGE(Vulkan)

	IF(NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
		SET(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
		STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
	ENDIF()

	IF(NOT Vulkan_FOUND)
		MESSAGE(STATUS "Failed to locate Vulkan SDK, retrying again...")
		IF(EXISTS "${VULKAN_PATH}")
			MESSAGE(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
		ELSE()
			MESSAGE("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
			MESSAGE("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
			RETURN()
		ENDIF()
	ENDIF()

	MESSAGE(STATUS "Attempting to locate Vulkan SDK using manual path......")
	SET(VULKAN_SDK "C:/VulkanSDK")
	SET(VULKAN_VERSION "1.3.243.0")
	#SET(VULKAN_VERSION "1.3.224.1")
	SET(VULKAN_PATH "${VULKAN_SDK}/${VULKAN_VERSION}")
	MESSAGE(STATUS "Using manual specified path: ${VULKAN_PATH}")

	IF(NOT EXISTS "${VULKAN_PATH}")
		MESSAGE("Error: Unable to locate this Vulkan SDK path VULKAN_PATH: ${VULKAN_PATH}, please specify correct path.
		For more information on correct installation process, please refer to subsection 'Getting started with Lunar-G SDK'
		and 'Setting up first project with CMake' in Chapter 3, 'Shaking hands with the device' in this book 'Learning Vulkan', ISBN - 9781786469809.")
	    RETURN()
	ENDIF()

ENDIF()

ADD_DEFINITIONS(-DVK_USE_PLATFORM_WIN32_KHR)

#SET(VULKAN_LIBS "vulkan-1;shaderc;shaderc_util;glslang;SPIRV;SPIRV-Tools;GenericCodeGen;MachineIndependent;SPIRV-Tools-opt;spirv-cross-core;spirv-cross-glsl;OGLCompiler;HLSL;OSDependent")
SET(VULKAN_LIBS "vulkan-1;shaderc;shaderc_util;glslang;SPIRV;SPIRV-Tools;GenericCodeGen;MachineIndependent;SPIRV-Tools-opt;OGLCompiler;HLSL;OSDependent")
SET(VULKAN_INCLUDES "${VULKAN_PATH}/Include")
SET(VULKAN_LIBS_DIR "${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib")
INCLUDE_DIRECTORIES(${VULKAN_INCLUDES})
LINK_DIRECTORIES(${VULKAN_LIBS_DIR})

# VMA
ADD_SUBDIRECTORY(ext/VMA)
INCLUDE_DIRECTORIES(ext/VMA/include)
SET(VMA_LIBS VulkanMemoryAllocator)

# ImGui
SET(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/")
FILE(GLOB IMGUI_SOURCES 
"${IMGUI_DIR}imgui.h"
"${IMGUI_DIR}imgui.cpp"

"${IMGUI_DIR}imgui_demo.cpp"
"${IMGUI_DIR}imgui_draw.cpp"
"${IMGUI_DIR}imgui_widgets.cpp"
"${IMGUI_DIR}imgui_tables.cpp"

"${IMGUI_DIR}backends/imgui_impl_vulkan.cpp"
"${IMGUI_DIR}backends/imgui_impl_glfw.cpp"
)
ADD_LIBRARY(imgui STATIC ${IMGUI_SOURCES})
INCLUDE_DIRECTORIES(${IMGUI_DIR})
SET(IMGUI_LIBS imgui)



# SPIRV-Cross
# ADD_SUBDIRECTORY(ext/SPIRV-Cross)
# INCLUDE_DIRECTORIES(ext/SPIRV-Cross)
# SET(SPV_CROSS_LIBS spirv-cross-glsl spirv-cross-cpp spirv-cross-reflect spirv-cross-util spirv-cross-core)

# VkEngine
SET(ALL_EXT_LIBS ${THAT_LIB} ${GLFW_LIBS} ${GLM_LIBS} ${VULKAN_LIBS} ${VMA_LIBS} ${IMGUI_LIBS} ${SPV_CROSS_LIBS})

ADD_DEFINITIONS(-DNOMINMAX)
SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

INCLUDE_DIRECTORIES(src)
INCLUDE_DIRECTORIES(ext)

# ADD_SUBDIRECTORY(src)

FILE(
	GLOB VK_ENGINE_SOURCE 
	src/Core/* 
	src/Core/App/*
	src/Core/Commands/*
	src/Core/Execution/*
	src/Core/IO/*
	src/Core/Modules/*
	src/Core/Rendering/*
	src/Core/VkObjects/*
	src/Core/Utils/* 
)

FILE(GLOB SPV_REFLECT_SOURCE ext/SPIRV-Reflect/*)

SOURCE_GROUP(TREE ${CMAKE_CURRENT_LIST_DIR}/src/ FILES ${VK_ENGINE_SOURCE})

SOURCE_GROUP("SPIRV-Reflect" FILES ${SPV_REFLECT_SOURCE})

ADD_LIBRARY(VkEngine STATIC ${VK_ENGINE_SOURCE} ${SPV_REFLECT_SOURCE})
SET_TARGET_PROPERTIES(VkEngine PROPERTIES LINKER_LANGUAGE CXX)
TARGET_COMPILE_DEFINITIONS(VkEngine PUBLIC "ENGINE_SRC_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/src/\"")
target_include_directories(VkEngine PUBLIC ext/thatlib/ext) # TODO do correctly


SET(EXECUTABLES)

function(AddProject p_name)
	if(DEFINED ARGV1)
		SET(folder ${ARGV1})
	else()
		SET(folder ${p_name})
	endif()
	FILE(GLOB ${p_name}_SOURCES src/${folder}/*)
	ADD_EXECUTABLE(${p_name} src/${folder}/main.cpp ${${p_name}_SOURCES})
	TARGET_INCLUDE_DIRECTORIES(${p_name} PRIVATE src/${folder})
	TARGET_COMPILE_DEFINITIONS(${p_name} PRIVATE "PROJECT_NAME=\"${p_name}\"")
	TARGET_COMPILE_DEFINITIONS(${p_name} PRIVATE "PROJECT_SRC_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/src/${folder}/\"")
	TARGET_LINK_LIBRARIES(${p_name} PUBLIC ${ALL_EXT_LIBS} VkEngine)
	SET(EXECUTABLES ${EXECUTABLES} ${p_name})
endfunction()

option(BUILD_PROJECTS ON)

if(${BUILD_PROJECTS})

	AddProject(Test)

	AddProject(GameOfLife "GOL")

	AddProject(Paint)

	AddProject(Particules)

	AddProject(Flotsam)

	AddProject(Renderer)

endif()

INSTALL(TARGETS VkEngine DESTINATION lib)

if(${BUILD_PROJECTS})

	INSTALL(TARGETS ${EXECUTABLES} DESTINATION bin)

endif()