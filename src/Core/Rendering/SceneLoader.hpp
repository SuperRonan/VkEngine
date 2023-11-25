#pragma once

#include <Core/Rendering/Scene.hpp>
#include <Core/Rendering/Model.hpp>

namespace vkl
{
	class NodeFromFile : public Scene::Node
	{
	protected:

		VkApplication * _app = nullptr;
		
		std::filesystem::path _path = {};

		std::shared_ptr<AsynchTask> _load_task = nullptr;

		std::vector<std::shared_ptr<Model>> _loaded_models = {};

	public:

		using Mat4 = glm::mat4;
		using Mat4x3 = glm::mat4x3;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Mat4x3 matrix = Mat4x3(1);
			std::filesystem::path path = {};
		};
		using CI = CreateInfo;

		NodeFromFile(CreateInfo const& ci);

		virtual ~NodeFromFile() override;

		virtual void updateResources(UpdateContext & ctx) override;
	};
}
