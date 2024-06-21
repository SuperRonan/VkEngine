
cmake_minimum_required(VERSION 3.18.0)

set(CMAKE_CONFIGURATION_TYPES Debug FastDebug RelWithDebInfo Release MinSizeRel)

set(Project_Name "VkEngine")
project(${Project_Name})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_FASTDEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
set(VK_ENGINE_LINK_OPTIONS "")
set(VK_ENGINE_LINK_OPTIONS ${VK_ENGINE_LINK_OPTIONS} "$<$<CONFIG:FastDebug>:/DEBUG>")

set(VK_ENGINE_INCLUDE_DIRS "")
set(VK_ENGINE_LINK_LIBS "")
set(VK_ENGINE_LINK_DEBUG_LIBS "")
set(VK_ENGINE_LINK_RELEASE_LIBS "")
set(VK_ENGINE_LINK_DIRS "")
set(VK_ENGINE_DEFINITIONS "")

# ThatLib
add_subdirectory(ext/thatlib)
set(THAT_LIB "thatlib")
set(THAT_LIB_INCLUDE_DIR ext/thatlib)
set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${THAT_LIB})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${THAT_LIB_INCLUDE_DIR})


# SDL (2)
# TODO Maybe not require SDL2 -> we can still use the engine, but without any functionality provided by SDL 
add_subdirectory(ext/SDL)
# set(SDL_DIRECTX OFF)
# set(SDL_OPENGL OFF)
# set(SDL_OPENGLES OFF)
# set(SDL_RENDER OFF)
# set(SDL_RENDERD3D OFF)
# set(SDL_VULKAN ON)
set(SDL2_LIBRARIES SDL2-static)
set(SDL2_INCLUDE_DIRS ext/SDL/include)
set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${SDL2_LIBRARIES})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${SDL2_INCLUDE_DIRS})

# GLM
add_subdirectory(ext/glm)
set(GLM_INCLUDE_DIRS ext/glm)
set(GLM_LIBS "")

set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${GLM_LIBS})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${GLM_INCLUDE_DIRS})

# Vulkan
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON)

if(AUTO_LOCATE_VULKAN)
	message(STATUS "Attempting auto locate Vulkan using CMake...")

	find_package(Vulkan)

	if(NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
		set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
		string(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
	endif()

	if(NOT Vulkan_FOUND)
		message(STATUS "Failed to locate Vulkan SDK, retrying again...")
		if(EXISTS "${VULKAN_PATH}")
			message(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
		else()
			message("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
			message("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
			return()
		endif()
	endif()

	message(STATUS "Attempting to locate Vulkan SDK using manual path......")
	set(VULKAN_SDK "C:/VulkanSDK")
	set(VULKAN_VERSION "1.3.283.0")
	# set(VULKAN_VERSION "1.3.250.0")
	# set(VULKAN_VERSION "1.3.243.0")
	# set(VULKAN_VERSION "1.3.224.1")
	set(VULKAN_PATH "${VULKAN_SDK}/${VULKAN_VERSION}")
	message(STATUS "Using manual specified path: ${VULKAN_PATH}")

	if(NOT EXISTS "${VULKAN_PATH}")
		message("Error: Unable to locate this Vulkan SDK path VULKAN_PATH: ${VULKAN_PATH}, please specify correct path.
		For more information on correct installation process, please refer to subsection 'Getting started with Lunar-G SDK'
		and 'Setting up first project with CMake' in Chapter 3, 'Shaking hands with the device' in this book 'Learning Vulkan', ISBN - 9781786469809.")
	    return()
	endif()

endif()

add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

set(VULKAN_LIBS "vulkan-1")

set(VULKAN_SHADER_LIBS_RELEASE "shaderc;shaderc_util;glslang;SPIRV;SPIRV-Tools;GenericCodeGen;MachineIndependent;SPIRV-Tools-opt;OSDependent")
set(VULKAN_SHADER_LIBS_DEBUG "shadercd;shaderc_utild;glslangd;SPIRVd;SPIRV-Toolsd;GenericCodeGend;MachineIndependentd;SPIRV-Tools-optd;OSDependentd")

set(VULKAN_INCLUDES "${VULKAN_PATH}/Include")
set(VULKAN_LIBS_DIR "${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib")

set(VK_ENGINE_LINK_DIRS "${VK_ENGINE_LINK_DIRS}" ${VULKAN_LIBS_DIR})
set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${VULKAN_LIBS})
set(VK_ENGINE_LINK_DEBUG_LIBS "${VK_ENGINE_LINK_DEBUG_LIBS}" ${VULKAN_SHADER_LIBS_DEBUG})
set(VK_ENGINE_LINK_RELEASE_LIBS "${VK_ENGINE_LINK_RELEASE_LIBS}" ${VULKAN_SHADER_LIBS_RELEASE})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${VULKAN_INCLUDES})

# VMA
add_subdirectory(ext/VMA)
set(VMA_INCLUDE_DIRS ext/VMA/include)
set(VMA_LIBS VulkanMemoryAllocator)

set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${VMA_LIBS})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${VMA_INCLUDE_DIRS})


# ImGui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/")
file(GLOB IMGUI_SOURCES 
"${IMGUI_DIR}imgui.h"
"${IMGUI_DIR}imgui.cpp"

"${IMGUI_DIR}imgui_demo.cpp"
"${IMGUI_DIR}imgui_draw.cpp"
"${IMGUI_DIR}imgui_widgets.cpp"
"${IMGUI_DIR}imgui_tables.cpp"
"${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"

"${IMGUI_DIR}backends/imgui_impl_vulkan.cpp"
"${IMGUI_DIR}backends/imgui_impl_sdl2.cpp"
"${IMGUI_DIR}backends/imgui_impl_win32.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${VULKAN_INCLUDES} ${SDL2_INCLUDE_DIRS})

set(IMGUI_LIBS imgui)

set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${IMGUI_LIBS})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${IMGUI_DIR})


# tinyobj
add_subdirectory(ext/tinyobj)
set(TINYOBJ_INCLUDE_DIRS ext)
SET(TINYOBJ_LIBS tinyobjloader)

set(VK_ENGINE_LINK_LIBS "${VK_ENGINE_LINK_LIBS}" ${TINYOBJ_LIBS})
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${TINYOBJ_INCLUDE_DIRS})


# ArgParse
add_subdirectory(ext/argparse)
set(ARGPARSE_INCLUDE_DIRS ext/argparse/include)
set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" ${ARGPARSE_INCLUDE_DIRS})

# SPIRV-Cross
# add_subdirectory(ext/SPIRV-Cross)
# include_directories(ext/SPIRV-Cross)
# set(SPV_CROSS_LIBS spirv-cross-glsl spirv-cross-cpp spirv-cross-reflect spirv-cross-util spirv-cross-core)

# VkEngine


add_definitions(-DNOMINMAX)
add_compile_definitions($<IF:$<CONFIG:Debug>,_ITERATOR_DEBUG_LEVEL=2,_ITERATOR_DEBUG_LEVEL=0>)

add_compile_definitions($<IF:$<CONFIG:Debug>,VKL_BUILD_DEBUG=1,VKL_BUILD_DEBUG=0>)
add_compile_definitions($<IF:$<CONFIG:FastDebug>,VKL_BUILD_FAST_DEBUG=1,VKL_BUILD_FAST_DEBUG=0>)
add_compile_definitions($<IF:$<CONFIG:RelWithDebInfo>,VKL_BUILD_RELEASE_WITH_DEBUG_INFO=1,VKL_BUILD_RELEASE_WITH_DEBUG_INFO=0>)
add_compile_definitions($<IF:$<CONFIG:Release>,VKL_BUILD_RELEASE=1,VKL_BUILD_RELEASE=0>)
add_compile_definitions($<IF:$<CONFIG:MinSizeRel>,VKL_BUILD_MIN_SIZE_RELEASE=1,VKL_BUILD_MIN_SIZE_RELEASE=0>)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(VK_ENGINE_INCLUDE_DIRS "${VK_ENGINE_INCLUDE_DIRS}" src ext)


FILE(
	GLOB VK_ENGINE_SOURCE 
	src/Core/* 
	src/Core/App/*
	src/Core/Commands/*
	src/Core/Execution/*
	src/Core/IO/*
	src/Core/Maths/*
	src/Core/Rendering/*
	src/Core/VkObjects/*
	src/Core/Utils/* 
)

file(GLOB SPV_REFLECT_SOURCE ext/SPIRV-Reflect/spirv_reflect.h ext/SPIRV-Reflect/spirv_reflect.cpp)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src/ FILES ${VK_ENGINE_SOURCE})

source_group("SPIRV-Reflect" FILES ${SPV_REFLECT_SOURCE})

set(ENGINE_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_library(VkEngine STATIC ${VK_ENGINE_SOURCE} ${SPV_REFLECT_SOURCE})
set_target_properties(VkEngine PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(VkEngine PUBLIC "ENGINE_SRC_PATH=\"${ENGINE_SRC_PATH}\"")
target_include_directories(VkEngine PUBLIC ${VK_ENGINE_INCLUDE_DIRS}) 

target_link_libraries(VkEngine PUBLIC ${VK_ENGINE_LINK_LIBS})
target_link_libraries(VkEngine PUBLIC $<IF:$<CONFIG:Debug>, ${VK_ENGINE_LINK_DEBUG_LIBS}, ${VK_ENGINE_LINK_RELEASE_LIBS}>)

target_link_directories(VkEngine PUBLIC ${VK_ENGINE_LINK_DIRS})



option(BUILD_PROJECTS ON)

if(${BUILD_PROJECTS})

add_subdirectory(projects)

endif()

install(TARGETS VkEngine DESTINATION lib)

# message(${STATIC_LIBRARY_FLAGS_DEBUG})
# message(${LINK_FLAGS_FASTDEBUG})
# message(${LINK_FLAGS_RELWITHDEBINFO})
# message(${LINK_FLAGS_RELEASE})
# message(${LINK_FLAGS_MINSIZEREL})