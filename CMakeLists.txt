
CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0)

SET(Project_Name "VkEngine")
PROJECT(${Project_Name})

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED TRUE)

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
	SET(VULKAN_VERSION "1.2.189.0")
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

SET(VULKAN_LIBS "vulkan-1")
SET(VULKAN_INCLUDES "${VULKAN_PATH}/Include")
SET(VULKAN_LIBS_DIR "${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib")

INCLUDE_DIRECTORIES(${VULKAN_INCLUDES})
LINK_DIRECTORIES(${VULKAN_LIBS_DIR})

# VkEngine


SET(ALL_LIBS ${VULKAN_LIBS})

INCLUDE_DIRECTORIES(src)

FILE(GLOB VK_ENGINE_SOURCE src/*)

ADD_LIBRARY(VkEngine STATIC ${VK_ENGINE_SOURCE})
SET_TARGET_PROPERTIES(VkEngine PROPERTIES LINKER_LANGUAGE CXX)

ADD_EXECUTABLE(App0 src/App0/main.cpp)
TARGET_LINK_LIBRARIES(App0 ${ALL_LIBS})

INSTALL(TARGETS VkEngine DESTINATION lib)

INSTALL(TARGETS App0 DESTINATION bin)

