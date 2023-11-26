#include "Scene.hpp"

#include <cassert>

#include <stack>
#include <Core/Utils/stl_extension.hpp>

#include <imgui/imgui.h>

namespace vkl
{

	void Scene::Node::updateResources(UpdateContext& ctx)
	{
		if (_model)
		{
			_model->updateResources(ctx);
		}
	}

	bool Scene::DirectedAcyclicGraph::checkIsAcyclic()const
	{
		// TODO
		return true;
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat4 const& matrix, const PerNodeInstanceFunction& f)
	{
		Mat4 new_matrix = matrix * node->matrix4x4();
		if (f(node, new_matrix))
		{
			for (std::shared_ptr<Node> const& n : node->children())
			{
				assert(!!n);
				iterateOnNodeThenSons(n, new_matrix, f);
			}
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceFunction& f)
	{
		Mat4 matrix = Mat4(1);
		if (root())
		{
			iterateOnNodeThenSons(root(), matrix, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeInstanceFunction& f)
	{
		for (auto& [node, matrices]  : _flat_dag)
		{
			for (const auto& matrix : matrices)
			{
				f(node, matrix);
			}
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeAllInstancesFunction& f)
	{
		for (auto& [node, matrices] : _flat_dag)
		{
			f(node, matrices);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodes(const PerNodeFunction& f)
	{
		for (auto& [node, matrices] : _flat_dag)
		{
			f(node);
		}
	}

	void Scene::DirectedAcyclicGraph::flatten()
	{
		_flat_dag.clear();

		const auto process_node = [&](std::shared_ptr<Node> const& node, Mat4 const& matrix)
		{
			std::vector<Mat4x3> & matrices = _flat_dag[node];
			matrices.push_back(Mat4x3(matrix));
			return true;
		};

		iterateOnDag(process_node);
	}

	Scene::DirectedAcyclicGraph::DirectedAcyclicGraph(std::shared_ptr<Node> root):
		_root(std::move(root))
	{
		
	}

	Scene::DirectedAcyclicGraph::PositionedNode Scene::DirectedAcyclicGraph::findNode(NodePath const& path) const
	{
		PositionedNode res;

		std::shared_ptr<Node> n = _root;
		Mat4 matrix = n->matrix4x4();
		for (size_t i = 0; i < path.path.size(); ++i)
		{
			if (path.path[i] < n->children().size())
			{
				n = n->children()[i];
				matrix *= n->matrix4x4();
			}
			else
			{
				n = nullptr;
				break;
			}
		}

		if (n)
		{
			res = PositionedNode{
				.node = n,
				.matrix = matrix,
			};
		}

		return res;
	}





	Scene::Scene(CreateInfo const& ci) :
		VkObject(ci.app, ci.name)
	{
		std::shared_ptr<Node> root = std::make_shared<Node>(Node::CI{.name = "root"});
		_tree = std::make_shared<DirectedAcyclicGraph>(root);

		createSet();
	}

	Scene::UBO Scene::getUBO()const
	{
		UBO res{
			.ambient = _ambient,
			.num_lights = static_cast<uint32_t>(_lights_glsl.size()),
		};
		return res;
	}



	std::shared_ptr<DescriptorSetLayout> Scene::SetLayout(VkApplication* app, SetLayoutOptions const& options)
	{
		std::shared_ptr<DescriptorSetLayoutCache>& gen_cache = app->getDescSetLayoutCache(static_cast<uint32_t>(DescriptorSetName::scene));
		if (!gen_cache)
		{
			gen_cache = std::make_shared< DescriptorSetLayoutCacheImpl<SetLayoutOptions>>();
		}
		std::shared_ptr<DescriptorSetLayoutCacheImpl<SetLayoutOptions>> cache = std::dynamic_pointer_cast<DescriptorSetLayoutCacheImpl<SetLayoutOptions>>(gen_cache);
		assert(!!cache);

		std::shared_ptr<DescriptorSetLayout> res = cache->findIFP(options);

		if (!res)
		{
			std::vector<DescriptorSetLayout::Binding> bindings;
			using namespace std::containers_operators;
			
			bindings += DescriptorSetLayout::Binding{
				.vk_binding = VkDescriptorSetLayoutBinding{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr,
				},
				.meta = DescriptorSetLayout::BindingMeta{
					.name = "SceneUBOBinding",
					.access = VK_ACCESS_2_UNIFORM_READ_BIT,
					.buffer_usage = VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT_KHR,
				},
			};

			bindings += DescriptorSetLayout::Binding{
				.vk_binding = VkDescriptorSetLayoutBinding{
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr,
				},
				.meta = DescriptorSetLayout::BindingMeta{
					.name = "LightsBufferBinding",
					.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					.buffer_usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT_KHR,
				},
			};


			res = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = app,
				.name = "Scene::SetLayout",
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
				.bindings = bindings,
				.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			});

			cache->recordValue(options, res);
		}

		return res;
	}

	std::shared_ptr<DescriptorSetLayout> Scene::setLayout()
	{
		if (!_set_layout)
		{
			_set_layout = SetLayout(application(), {});
		}
		return _set_layout;
	}

	void Scene::createInternalBuffers()
	{
		_ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".UBOBuffer",
			.size = sizeof(UBO),
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_lights_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".LightBuffer",
			.size = [this](){return std::max(_lights_glsl.size() * sizeof(LightGLSL), 1ull);},
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
	}

	void Scene::createSet()
	{
		createInternalBuffers();
		using namespace std::containers_operators;
		std::shared_ptr<DescriptorSetLayout> layout = setLayout();
		ShaderBindings bindings;

		bindings += Binding{
			.buffer = _ubo_buffer,
			.binding = 0,
		};

		bindings += Binding{
			.buffer = _lights_buffer,
			.binding = 1,
		};

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CI{
			.app = application(),
			.name = name() + ".set",
			.layout = layout ,
			.bindings = bindings,
		});
	}

	std::shared_ptr<DescriptorSetAndPool> Scene::set()
	{
		return _set;
	}

	void Scene::fillLightsBuffer()
	{
		_lights_glsl.clear();
		
		_tree->iterateOnFlattenDag([&](std::shared_ptr<Node> const& node, Mat4 const& matrix)
		{
			if (node->light())
			{
				const Light & l = *node->light();
				LightGLSL gl = l.getAsGLSL(matrix);
				_lights_glsl.push_back(gl);
			}
			return true;
		});
	}

	void Scene::prepareForRendering()
	{
		_tree->flatten();
		fillLightsBuffer();
	}

	void Scene::updateResources(UpdateContext& ctx)
	{
		// Maybe separate between the few scene own internal resources and the lot of nodes resources (models, textures, ...)
		_ubo_buffer->updateResource(ctx);
		_lights_buffer->updateResource(ctx);
		_set->updateResources(ctx);

		_tree->iterateOnNodes([&](std::shared_ptr<Node> const& node)
		{
			node->updateResources(ctx);
		});

		{
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = {
					PositionedObjectView{
						.obj = getUBO(),
						.pos = 0,
					},
				},
				.dst = _ubo_buffer,
			};

			if (!_lights_glsl.empty())
			{
				ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
					.sources = {
						PositionedObjectView{
							.obj = _lights_glsl,
							.pos = 0,
						},
					},
					.dst = _lights_buffer,
				};
			}
		}
	}

	void Scene::declareGui(GuiContext & ctx)
	{
		ImGui::PushID(name().c_str());
		if (ImGui::CollapsingHeader("Options"))
		{
			ImGui::ColorPicker3("Ambient", &_ambient.x);
		}

		if (ImGui::CollapsingHeader("Tree"))
		{
			DAG::NodePath path;
			auto declare_node = [&](std::shared_ptr<Node> const& node, Mat4 const& matrix, bool is_selected_path_so_far, const auto & recurse) -> void
			{
				Mat4 node_matrix = matrix * node->matrix4x4();
				std::string node_gui_name = node->name();

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
				if(!path.path.empty())
				{
					flags |= ImGuiTreeNodeFlags_OpenOnArrow;
				}
				const bool is_leaf = node->children().empty();
				if (is_leaf)
				{
					flags |= ImGuiTreeNodeFlags_Leaf;
				}

				bool is_selected = false;
				if (is_selected_path_so_far)
				{
					if (!path.path.empty())
					{
						if (_gui_selected_node.path.path.size() >= path.path.size())
						{
							if (_gui_selected_node.path.path[path.path.size() - 1] != path.path.back())
							{
								is_selected_path_so_far = false;
							}
							else
							{
								is_selected = (_gui_selected_node.path.path.size() == path.path.size());
							}
						}
						else
						{
							is_selected_path_so_far = false;
						}
					}
				}

				if (is_selected || node == _gui_selected_node.node.node)
				{
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				const bool node_open = ImGui::TreeNodeEx(node_gui_name.c_str(), flags);
				
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && !path.path.empty())
				{
					std::cout << "Clicked " << node->name() << std::endl;
					_gui_selected_node = SelectedNode{
						.node = DAG::PositionedNode{
							.node = node,
							.matrix = node_matrix,
						},
						.path = path,
					};
				}

				if(node_open)
				{	
					path.path.push_back(0);
					for (size_t i = 0; i < node->children().size(); ++i)
					{
						path.path.back() = i;
						recurse(node->children()[i], node_matrix, is_selected_path_so_far, recurse);
					}
					path.path.pop_back();

					ImGui::TreePop();
				}
			};

			Mat4 root_matrix = Mat4(1);
			declare_node(_tree->root(), root_matrix, true, declare_node);
		} // Tree


		if (ImGui::Begin("Node Inspector"))
		{
			if (_gui_selected_node.node.node)
			{
				std::shared_ptr node = _gui_selected_node.node.node;
				ImGui::PushID("Node Inspector");
				ImGui::Text(node->name().c_str());

				ImGui::SameLine();
				if (ImGui::Button("Close"))
				{
					_gui_selected_node.clear();
				}
				else
				{
					if (ImGui::CollapsingHeader("Transform"))
					{
					
					}

					if (ImGui::CollapsingHeader("Model"))
					{
						if (node->model())
						{
						
						}
						else
						{
							ImGui::Text("None");
							//ImGui::BeginDragDropTarget();

							//ImGui::EndDragDropTarget();
						}
					}
				}


				ImGui::PopID();
			}
			ImGui::End();
		}

		ImGui::PopID();
	}
}