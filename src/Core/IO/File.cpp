#include <Core/IO/File.hpp>

#include <fstream>
#include <thread>

namespace vkl
{
	std::vector<uint8_t> ReadFile(std::filesystem::path const& path)
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

	std::string ReadFileToString(std::filesystem::path const& path)
	{
		std::ifstream file;
		int tries = 0;
		const int max_tries = 1;
		do
		{
			if (tries)
			{
				std::this_thread::sleep_for(1us);
			}
			file = std::ifstream(path, std::ios::ate | std::ios::binary);
			++tries;
		} while (!file.is_open() && tries != max_tries);

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

	void WriteFile(std::filesystem::path const& path, ObjectView const& view)
	{
		std::ofstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Could not open file: " + path.string());
		}

		file.write((const char*)view.data(), view.size());
		file.close();
	}

	std::filesystem::path GetCurrentExecutableAbsolutePath()
	{
		std::filesystem::path res;
#if _WIN32
		const DWORD cap = 1024;
		CHAR win_path[cap];
		DWORD _res = GetModuleFileName(NULL, win_path, cap);
		if (_res > 0)
		{
			res = win_path;
		}
#endif
		return std::filesystem::absolute(res);
	}
}