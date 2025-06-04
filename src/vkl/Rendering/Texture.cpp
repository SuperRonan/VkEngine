#include <vkl/Rendering/Texture.hpp>
#include <vkl/Rendering/TextureFromFile.hpp>

namespace vkl
{
	void Texture::registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index)
	{
		DescriptorSetAndPool::Registration reg{
			.set = set,
			.binding = binding,
			.array_index = array_index,
		};
		callRegistrationCallback(reg);
		_registered_sets.push_back(std::move(reg));
	}

	void Texture::unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t index)
	{
		DescriptorSetAndPool::Registration target{
			.set = set,
			.binding = binding,
			.array_index = index,
		};
		// Assume present only once
		auto it = _registered_sets.begin();
		while (it != _registered_sets.end())
		{
			const auto & reg = *it;
			if (reg == target)
			{
				std::shared_ptr<ImageView> null_image = nullptr;
				std::shared_ptr<Sampler> null_sampler = nullptr;
				reg.set->setBinding(reg.binding, reg.array_index, 1, &null_image, &null_sampler);
				it = _registered_sets.erase(it);
				break;
			}
			else
			{
				++it;
			}
		}
	}

	void Texture::unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set)
	{
		auto it = _registered_sets.begin();
		while (it != _registered_sets.end())
		{
			const auto& reg = *it;
			if (reg.set == set)
			{
				reg.set->setBinding(reg.binding, reg.array_index, 1, nullptr, nullptr);
				it = _registered_sets.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void Texture::callRegistrationCallback(DescriptorSetAndPool::Registration & reg)
	{
		reg.set->setBinding(reg.binding, reg.array_index, 1, &_view, nullptr);
	}

	void Texture::callResourceUpdateCallbacks()
	{
		for (size_t i = 0; i < _registered_sets.size(); ++i)
		{
			callRegistrationCallback(_registered_sets[i]);
		}
	}

	void Texture::declareGUI(GuiContext& ctx)
	{
		
	}



	std::shared_ptr<Texture> Texture::MakeShared(MakeInfo const& mi)
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