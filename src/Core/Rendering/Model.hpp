#pragma once

#include "Mesh.hpp"
#include <Core/Execution/DescriptorSetsManager.hpp>

namespace vkl
{
	class Model : public VkObject
	{
	protected:
		
		std::shared_ptr<Mesh> _mesh = nullptr;


		std::shared_ptr<DescriptorSetAndPool> _desc_set;


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<Mesh> mesh = nullptr;
		};
		using CI = CreateInfo;

		Model(CreateInfo const& ci);

		std::shared_ptr<Mesh> mesh()const
		{
			return _mesh;
		}

	};
}