#pragma once

#include <filesystem>

#include <Core/VulkanCommons.hpp>

namespace vkl
{
	std::vector<uint8_t> ReadFile(std::filesystem::path const& path);

	std::string ReadFileToString(std::filesystem::path const& path);

	void WriteFile(std::filesystem::path const& path, ObjectView const& view);

	std::filesystem::path GetCurrentExecutableAbsolutePath();
}