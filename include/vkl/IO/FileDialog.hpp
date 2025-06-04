#pragma once

#include <vkl/Core/VulkanCommons.hpp>

#include <that/IO/FileSystem.hpp>

#include <condition_variable>

namespace vkl
{
	class FileDialog
	{
	public:

		enum class Mode {
			OpenFile,
			OpenFolder,
			SaveFile
		};

		using Path = that::FileSystem::Path;

		struct OpenInfo
		{
			std::span<SDL_DialogFileFilter> filters;
			Path default_location = {};
			SDL_Window* parent_window = nullptr;
			bool allow_multiple = false;
			Mode mode = Mode::OpenFile;
		};

	protected:

		that::ExtensibleStringStorage _string_storage;
		MyVector<SDL_DialogFileFilter> _filters;
		
		std::mutex _mutex = {};
		std::condition_variable _cv = {};

		const void* _owner = nullptr;

		MyVector<Path> _results = {};

		int _result_filter = 0;
		bool _completed = false;
		bool _is_open = false;

	public:
		
		struct CreateInfo
		{

		};
		using CI = CreateInfo;

		FileDialog(CreateInfo const& ci);

		~FileDialog();

		const void* owner() const
		{
			return _owner;
		}

		bool canOpen() const
		{
			return !_is_open;
		}

		void open(const void* owner, OpenInfo const& info);

		void close();

		bool completed() const;

		void waitOnCompletion();

		int selectedFilterIndex() const
		{
			return _result_filter;
		}

		std::span<const Path> getResults() const
		{
			return std::span<const Path>(_results.data(), _results.size());
		}

		auto& resultVector()
		{
			return _results;
		}
	};
}