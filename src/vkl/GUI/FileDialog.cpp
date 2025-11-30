#include <vkl/GUI/FileDialog.hpp>

#include <iostream>

namespace vkl
{
	FileDialog::FileDialog(CreateInfo const& ci) {}

	FileDialog::~FileDialog() {}

	void FileDialog::open(const void* owner, OpenInfo const& info)
	{
		assertm(!_is_open, "Cannot open a FileDialog that is already opened");
		_results.clear();
		_is_open = true;
		_owner = owner;
		_completed = false;
		_result_filter = 0;

		auto callback = [](void * user_data, const char * const * file_list, int filter)
		{
			FileDialog& that = *reinterpret_cast<FileDialog*>(user_data);
			that._result_filter = filter;
			const char * const * it = file_list;
			while (it && *it)
			{
				if (*it)
				{
					that._results.push_back(Path(*it));
					++it;
				}
			}
			that._completed = true;
			std::unique_lock lock(that._mutex);
			that._cv.notify_all();
		};
		
		// TODO
		// if default location does not end with a '/', it will not be interpretted correctly
		// For example if def loc is: A/B/C
		// We expect to open the dialog in folder 'C'
		// But SDL will open at 'B' and suggest to select file 'C'
		size_t default_location_id = _string_storage.pushBack(info.default_location.string());
		
		if (info.mode != Mode::OpenFolder)
		{
			_filters.resize(info.filters.size());
			for (size_t i = 0; i < _filters.size(); ++i)
			{
				size_t name_index = _string_storage.pushBack(info.filters[i].name);
				size_t pattern_index = _string_storage.pushBack(info.filters[i].pattern);
				SDL_DialogFileFilter tmp;
				tmp.name = std::bit_cast<const char*>(uintptr_t(name_index));
				tmp.pattern = std::bit_cast<const char*>(uintptr_t(pattern_index));
				_filters[i] = tmp;
			}
			for (size_t i = 0; i < _filters.size(); ++i)
			{
				auto& tmp = _filters[i];
				tmp.name = _string_storage.data() + std::bit_cast<uintptr_t>(tmp.name);
				tmp.pattern = _string_storage.data() + std::bit_cast<uintptr_t>(tmp.pattern);
			}
		}
		
		if (info.mode == Mode::OpenFile)
		{
			SDL_ShowOpenFileDialog(callback, this, info.parent_window, _filters.data(), _filters.size(), _string_storage.data() + default_location_id, info.allow_multiple);
		}
		else if (info.mode == Mode::OpenFolder)
		{
			SDL_ShowOpenFolderDialog(callback, this, info.parent_window, _string_storage.data() + default_location_id, info.allow_multiple);
		}
		else if (info.mode == Mode::SaveFile)
		{
			SDL_ShowSaveFileDialog(callback, this, info.parent_window, _filters.data(), _filters.size(), _string_storage.data() + default_location_id);
		}
	}

	void FileDialog::close()
	{
		_owner = nullptr;
		_is_open = false;
		_string_storage.clear();
		_filters.clear();
	}

	void FileDialog::waitOnCompletion()
	{
		std::unique_lock lock(_mutex);
		_cv.wait(lock);
	}

	bool FileDialog::completed() const
	{
		return _completed && _is_open;
	}

	
}