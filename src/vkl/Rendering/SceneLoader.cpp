#include <vkl/Rendering/SceneLoader.hpp>

namespace vkl
{
	void NodeFromFile::createChildrenFromLoadedModels()
	{
		if (_loaded_models.size() == 1)
		{
			_model = _loaded_models[0];
		}
		else if (_loaded_models.size() > 1)
		{
			_children.reserve(_children.size() + _loaded_models.size());
			for (auto& lm : _loaded_models)
			{
				std::shared_ptr<Scene::Node> n = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = lm->name(),
					.model = lm,
					});
				_children.push_back(n);
			}
		}
		_loaded_models.clear();
	}

	NodeFromFile::NodeFromFile(CreateInfo const& ci):
		Scene::Node(Scene::Node::CI{
			.name = ci.name,
			.matrix = ci.matrix,
		}),
		_app(ci.app),
		_path(ci.path),
		_synch(ci.synch)
	{
		_visible = false;
		if (!_path.empty())
		{
			assert(_app);
			if (isSynch())
			{
				_loaded_models = Model::loadModelsFromObj(Model::LoadInfo{
					.app = _app,
					.path = _path,
					.synch = _synch,
				});
				createChildrenFromLoadedModels();
				_visible = true;
			}
			else
			{
				_load_task = std::make_shared<AsynchTask>(AsynchTask::CI{
					.name = name() + ".LoadFromFile()",
					.priority = TaskPriority::WhenPossible(),
					.lambda = [this]() {

						//std::this_thread::sleep_for(5s);
					
						_loaded_models = Model::loadModelsFromObj(Model::LoadInfo{
							.app = _app,
							.path = _path,
							.synch = _synch,
						});

						AsynchTask::ReturnType res{
							.success = true,
						};
						return res;
					},
				});	
				_app->threadPool().pushTask(_load_task);
			}
		}
	}

	NodeFromFile::~NodeFromFile()
	{
		if (_load_task)
		{
			_load_task->waitIFN();
		}
	}

	void NodeFromFile::updateResources(UpdateContext& ctx)
	{
		if (_load_task)
		{
			if (_load_task->StatusIsFinish(_load_task->getStatus()))
			{
				if (_load_task->isSuccess())
				{
					createChildrenFromLoadedModels();
					_visible = true;
				}
				else
				{

				}
				_load_task = nullptr;
			}
		}

		Scene::Node::updateResources(ctx);
	}




	std::shared_ptr<Scene::Node> MakeLightNode(LightNodeCreateInfo const& ci)
	{
		std::shared_ptr<Scene::Node> res;
		res = std::make_shared<Scene::Node>(Scene::Node::CI{
			.name = ci.name,
			.matrix = ci.xform,
		});

		if (ci.type == LightType::POINT)
		{
			res->light() = std::make_shared<PointLight>(PointLight::CI{
				.app = ci.app,
				.name = ci.name,
				.emission = ci.emission,
				.enable_shadow_map = ci.enable_shadow_map,
			});
		}
		else if (ci.type == LightType::DIRECTIONAL)
		{
			res->light() = std::make_shared<DirectionalLight>(DirectionalLight::CI{
				.app = ci.app,
				.name = ci.name,
				.direction = Vector3f(0, 1, 0),
				.emission = ci.emission,
			});
		}
		else if (ci.type == LightType::SPOT)
		{
			res->light() = std::make_shared<SpotLight>(SpotLight::CI{
				.app = ci.app,
				.name = ci.name,
				.emission = ci.emission,
				.enable_shadow_map = ci.enable_shadow_map,
			});
		}
		return res;
	}

	std::shared_ptr<Scene::Node> MakeModelNode(
		std::string_view name,
		std::shared_ptr<Mesh> const& mesh,
		std::shared_ptr<Material> const& material,
		AffineXForm3Df const& xform
	) {
		std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
			.app = mesh->application(),
			.name = std::format("{}.Model", name),
			.mesh = mesh,
			.material = material,
		});
		std::shared_ptr<Scene::Node> res = std::make_shared<Scene::Node>(Scene::Node::CI{
			.name = std::string(name),
			.matrix = xform,
			.model = model,
		});
		return res;
	}

	std::shared_ptr<Scene::Node> MakeModelNode(BasicModelNodeCreateInfo const& ci)
	{
		std::shared_ptr<Mesh> mesh = RigidMesh::MakeRigidMesh(RigidMesh::RigidMeshMakeInfo{
			.app = ci.app,
			.name = std::format("{}.Mesh", ci.name),
			.type = ci.mesh_type,
			.subdivisions = ci.subdivisions,
			.face_normal = true,
		});

		std::shared_ptr<PBMaterial> material = std::make_shared<PBMaterial>(PBMaterial::CI{
			.app = ci.app,
			.name = std::format("{}.Material", ci.name),
			.albedo = ci.albedo,
			.metallic = ci.metallic_or_eta,
			.roughness = ci.roughness,
			.cavity = 0,
			.is_dielectric = ci.is_dielectric,
			.sample_spectral = ci.sample_spectral,
		});

		return MakeModelNode(ci.name, mesh, material, ci.xform);
	}

	
}