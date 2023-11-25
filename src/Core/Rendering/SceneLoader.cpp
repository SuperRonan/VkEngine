#include "SceneLoader.hpp"

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
}