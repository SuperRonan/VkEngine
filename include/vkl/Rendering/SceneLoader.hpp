#pragma once

#include <vkl/Rendering/Scene.hpp>
#include <vkl/Rendering/Model.hpp>

namespace vkl
{
	class NodeFromFile : public Scene::Node
	{
	protected:

		VkApplication * _app = nullptr;
		
		std::filesystem::path _path = {};

		std::shared_ptr<AsynchTask> _load_task = nullptr;

		std::vector<std::shared_ptr<Model>> _loaded_models = {};

		bool _synch = true;

		void createChildrenFromLoadedModels();

	public:

		using Mat4 = Matrix4f;
		using XForm = AffineXForm3D<float>;

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			XForm matrix = XForm::Identity();
			std::filesystem::path path = {};
			bool synch = true;
		};
		using CI = CreateInfo;

		NodeFromFile(CreateInfo const& ci);

		virtual ~NodeFromFile() override;

		virtual void updateResources(UpdateContext & ctx) override;

		constexpr bool isSynch()const
		{
			return _synch;
		}
	};

	struct LightNodeCreateInfo
	{
		VkApplication * app = nullptr;
		std::string name = {};
		AffineXForm3Df xform = AffineXForm3Df::Identity();
		LightType type = LightType::None;
		Vector3f emission = Vector3f::Ones().eval();
		bool enable_shadow_map;
	};
	std::shared_ptr<Scene::Node> MakeLightNode(LightNodeCreateInfo const& ci);

	struct BasicModelNodeCreateInfo
	{
		VkApplication* app = nullptr;
		std::string name = {};

		AffineXForm3Df xform = AffineXForm3Df::Identity();

		RigidMesh::RigidMeshMakeInfo::Type mesh_type = {};
		uvec4 subdivisions {};

		vec3 albedo = {};
		float roughness = {};
		float metallic_or_eta = {};
		bool is_dielectric = {};
	};

	std::shared_ptr<Scene::Node> MakeModelNode(BasicModelNodeCreateInfo const& ci);
}
