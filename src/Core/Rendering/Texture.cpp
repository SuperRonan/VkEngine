#include "Texture.hpp"
#include "TextureFromFile.hpp"

namespace vkl
{
	void Texture::addResourceUpdateCallback(Callback const& cb)
	{
		_resource_update_callback.push_back(cb);
	}

	void Texture::removeResourceUpdateCallback(VkObject* id)
	{
		for (size_t i = 0; i < _resource_update_callback.size(); ++i)
		{
			if (_resource_update_callback[i].id == id)
			{
				_resource_update_callback.erase(_resource_update_callback.begin() + i);
				break;
			}
		}
	}

	void Texture::callResourceUpdateCallbacks()
	{
		for (size_t i = 0; i < _resource_update_callback.size(); ++i)
		{
			_resource_update_callback[i].callback();
		}
	}



	std::shared_ptr<Texture> Texture::MakeNew(MakeInfo const& mi)
	{
		std::shared_ptr<TextureFromFile> res = std::make_shared<TextureFromFile>(TextureFromFile::CI{
			.app = mi.app,
			.name = mi.name,
			.path = mi.path,
			.desired_format = mi.desired_format,
			.synch = mi.synch,
		});

		return res;
	}
}