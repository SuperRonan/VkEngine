#include "Scene.hpp"

#include <cassert>
#include <stack>
#include <bitset>

#include <Core/Utils/stl_extension.hpp>

#include <imgui/imgui.h>
#include <Core/IO/ImGuiUtils.hpp>

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

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, FastNodePath & path, Mat4 const& matrix, const PerNodeInstanceFastPathFunction& f)
	{
		Mat4 new_matrix = matrix * node->matrix4x4();
		if (f(node, path, new_matrix))
		{
			path.path.push_back(0);
			for (size_t i=0; i < node->children().size(); ++i)
			{
				path.path.back() = static_cast<uint32_t>(i);
				std::shared_ptr<Node> const& n = node->children()[i];
				assert(!!n);
				iterateOnNodeThenSons(n, path, new_matrix, f);
			}
			path.path.pop_back();
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, RobustNodePath& path, Mat4 const& matrix, uint32_t flags, const PerNodeInstanceRobustPathFunction& f)
	{
		Mat4 new_matrix = matrix * node->matrix4x4();
		bool visible = node->visible();
		std::bitset<32> flags_bits(flags);
		flags_bits.set(0, visible && flags_bits[0]);
		uint32_t new_flags = flags_bits.to_ulong();
		if (f(node, path, new_matrix, new_flags))
		{
			path.path.push_back(nullptr);

			for (size_t i = 0; i < node->children().size(); ++i)
			{
				std::shared_ptr<Node> const& n = node->children()[i];
				assert(!!n);
				path.path.back() = n.get();
				iterateOnNodeThenSons(n, path, new_matrix, new_flags, f);
			}
			path.path.pop_back();
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

	void Scene::DirectedAcyclicGraph::iterateOnDag(std::function<bool(std::shared_ptr<Node> const&, FastNodePath const& path, Mat4 const& matrix)> const& f)
	{
		FastNodePath path;
		Mat4 matrix = Mat4(1);
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnDag(PerNodeInstanceRobustPathFunction const& f)
	{
		RobustNodePath path;
		Mat4 matrix = Mat4(1);
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, 1, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeInstanceFunction& f)
	{
		for (auto& [node, matrices]  : _flat_dag)
		{
			for (const auto& matrix : matrices)
			{
				f(node, matrix.matrix);
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
			std::vector<PerNodeInstance> & matrices = _flat_dag[node];
			matrices.push_back(PerNodeInstance{
				.matrix = matrix, 
				.visible = true, // TODO
			});
			return true;
		};

		iterateOnDag(process_node);
	}

	Scene::DirectedAcyclicGraph::DirectedAcyclicGraph(std::shared_ptr<Node> root):
		_root(std::move(root))
	{
		
	}

	size_t Scene::DirectedAcyclicGraph::FastNodePath::hash()const
	{
		size_t res = 0;
		for (size_t i = 0; i < path.size(); ++i)
		{
			res = std::hash<size_t>()(res ^ std::hash<uint32_t>()(path[i]));
		}
		return res;
	}

	size_t Scene::DirectedAcyclicGraph::RobustNodePath::hash()const
	{
		size_t res = 0;
		for (size_t i = 0; i < path.size(); ++i)
		{
			res = std::hash<size_t>()(res ^ std::hash<void*>()(path[i]));
		}
		return res;
	}

	Scene::DirectedAcyclicGraph::PositionedNode Scene::DirectedAcyclicGraph::findNode(FastNodePath const& path) const
	{
		PositionedNode res;

		std::shared_ptr<Node> n = _root;
		Mat4 matrix = n->matrix4x4();
		for (size_t i = 0; i < path.path.size(); ++i)
		{
			if (path.path[i] < n->children().size())
			{
				n = n->children()[path.path[i]];
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

	Scene::DirectedAcyclicGraph::PositionedNode Scene::DirectedAcyclicGraph::findNode(RobustNodePath const& path) const
	{
		PositionedNode res;

		std::shared_ptr<Node> n = _root;
		Mat4 matrix = n->matrix4x4();
		for (size_t i = 0; i < path.path.size(); ++i)
		{
			size_t found = size_t(-1);
			for (size_t j = 0; j < n->children().size(); ++j)
			{
				if (n->children()[j].get() == path.path[i])
				{
					found = j;
					break;
				}
			}
			if (found != size_t(-1))
			{
				n = n->children()[found];
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
			.num_lights = _num_lights,
			.ambient = _ambient,
		};
		return res;
	}



	//std::shared_ptr<DescriptorSetLayout> Scene::SetLayout(VkApplication* app, SetLayoutOptions const& options)
	//{
	//	std::shared_ptr<DescriptorSetLayoutCache> gen_cache = app->getDescSetLayoutCacheOrEmplace(static_cast<uint32_t>(DescriptorSetName::scene), []()
	//	{
	//		return std::make_shared< DescriptorSetLayoutCacheImpl<SetLayoutOptions>>();
	//	});
	//	std::shared_ptr<DescriptorSetLayoutCacheImpl<SetLayoutOptions>> cache = std::dynamic_pointer_cast<DescriptorSetLayoutCacheImpl<SetLayoutOptions>>(gen_cache);
	//	assert(!!cache);

	//	std::shared_ptr<DescriptorSetLayout> res = cache->findOrEmplace(options, [app]() {

	//		
	//		return res;
	//	});

	//	return res;
	//}

	std::shared_ptr<DescriptorSetLayout> Scene::setLayout()
	{
		if (!_set_layout)
		{
			_lights_bindings_base = 1;
			const uint32_t lights_num_bindings = 3;
			_objects_binding_base = _lights_bindings_base + lights_num_bindings;
			const uint32_t objects_num_bindings = 1;
			_mesh_bindings_base = _objects_binding_base + objects_num_bindings;
			const uint32_t mesh_num_bindings = 3;
			_material_bindings_base = _mesh_bindings_base + mesh_num_bindings;
			const uint32_t material_num_bindings = 2;
			_textures_bindings_base = _material_bindings_base + material_num_bindings;
			const uint32_t textures_num_bindings = 1;
			_xforms_bindings_base = _textures_bindings_base + textures_num_bindings;
			const uint32_t xforms_num_bindings = 2;


			std::vector<DescriptorSetLayout::Binding> bindings;
			using namespace std::containers_append_operators;

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneUBOBinding",
				.binding = 0,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_UNIFORM_READ_BIT,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			};

			bindings += DescriptorSetLayout::Binding{
				.name = "LightsBufferBinding",
				.binding = _lights_bindings_base + 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			Dyn<uint32_t> light_depth_map_2D_count = [this](){return _light_depth_map_2D_index_pool.capacity();};
			bindings += DescriptorSetLayout::Binding{
				.name = "LightsDepth2D",
				.binding = _lights_bindings_base + 1,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.count = light_depth_map_2D_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			};
			Dyn<uint32_t> light_depth_map_cube_count = [this](){return _light_depth_map_cube_index_pool.capacity();};
			bindings += DescriptorSetLayout::Binding{
				.name = "LightsDepthCube",
				.binding = _lights_bindings_base + 2,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.count = light_depth_map_cube_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			};

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneObjectsTable",
				.binding = _objects_binding_base + 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			Dyn<uint32_t> mesh_count = [this](){return _unique_mesh_index_pool.capacity();};

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneMeshHeadersBindings",
				.binding = _mesh_bindings_base + 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = mesh_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneMeshVerticesBindings",
				.binding = _mesh_bindings_base + 1,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = mesh_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneMeshIndicesBindings",
				.binding = _mesh_bindings_base + 2,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = mesh_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			Dyn<uint32_t> material_count = [this](){return _unique_material_index_pool.capacity();};
			
			bindings += DescriptorSetLayout::Binding{
				.name = "ScenePBMaterialsBinding",
				.binding = _material_bindings_base + 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = material_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			bindings += DescriptorSetLayout::Binding{
				.name = "ScenePBMaterialsRefBinding",
				.binding = _material_bindings_base + 1,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};
			
			Dyn<uint32_t> texture_2D_count = [this](){return _unique_texture_2D_index_pool.capacity();};

			bindings += DescriptorSetLayout::Binding{
				.name = "SceneTextures2D",
				.binding = _textures_bindings_base + 0,
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.count = texture_2D_count,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			};



			bindings += DescriptorSetLayout::Binding{
				.name = "SceneXFormBinding",
				.binding = _xforms_bindings_base + 0,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};
			
			bindings += DescriptorSetLayout::Binding{
				.name = "ScenePrevXFormBinding",
				.binding = _xforms_bindings_base + 1,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.count = 1,
				.stages = VK_SHADER_STAGE_ALL,
				.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			_set_layout = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = application(),
				.name = name() + ".SetLayout",
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
				.is_dynamic = true,
				.bindings = bindings,
				.binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
			});
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

		_model_references_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".ModelReferences",
			.size = sizeof(ModelReference) * 256,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_lights_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".LightBuffer",
			.size = sizeof(LightGLSL) * 1,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_material_ref_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".MaterialRefBuffer",
			.size = sizeof(MaterialReference) * 256,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_xforms_buffer = std::make_shared<HostManagedBuffer>(HostManagedBuffer::CI{
			.app = application(),
			.name = name() + ".xforms",
			.size = sizeof(Mat4x3) * 256,
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_prev_xforms_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".prev_xforms",
			.size = [this](){return _xforms_buffer->byteSize();},
			.usage = VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
		_xforms_segment = BufferAndRange{
			.buffer = _xforms_buffer->buffer(),
		};
		_prev_xforms_segment = BufferAndRange{
			.buffer = _prev_xforms_buffer,
		};
	}

	void Scene::createSet()
	{
		createInternalBuffers();
		using namespace std::containers_append_operators;
		std::shared_ptr<DescriptorSetLayout> layout = setLayout();
		ShaderBindings bindings;

		bindings += Binding{
			.buffer = _ubo_buffer,
			.binding = 0,
		};

		bindings += Binding{
			.buffer = _lights_buffer->buffer(),
			.binding = _lights_bindings_base + 0,
		};

		bindings += Binding{
			.buffer = _model_references_buffer->buffer(),
			.binding = _objects_binding_base + 0,
		};

		bindings += Binding{
			.buffer = _material_ref_buffer->buffer(),
			.binding = _material_bindings_base + 1,
		};

		bindings += Binding{
			.buffer = _xforms_segment.buffer,
			.buffer_range = _xforms_segment.range,
			.binding = _xforms_bindings_base + 0,
		};

		bindings += Binding{
			.buffer = _prev_xforms_segment.buffer,
			.buffer_range = _prev_xforms_segment.range,
			.binding = _xforms_bindings_base + 1,
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
		_num_lights = 0;	
		
		_tree->iterateOnFlattenDag([&](std::shared_ptr<Node> const& node, Mat4 const& matrix)
		{
			if (node->light())
			{
				const Light & l = *node->light();
				LightGLSL gl = l.getAsGLSL(matrix);
				//_lights_glsl.push_back(gl);
			}
			return true;
		});
	}

	void Scene::updateInternal()
	{
		_tree->_flat_dag.clear();
		_num_lights = 0;

		std::unordered_map<DirectedAcyclicGraph::RobustNodePath, size_t> m;
		m[DirectedAcyclicGraph::RobustNodePath()] = 21;
		std::hash<DirectedAcyclicGraph::RobustNodePath> h;
		static_assert(std::concepts::HashableFromMethod<DirectedAcyclicGraph::RobustNodePath>);

		_tree->iterateOnDag([&](std::shared_ptr<Node> const& node, DirectedAcyclicGraph::RobustNodePath const& path, Mat4 const& matrix4, uint32_t flags)
		{
			const Mat4x3 matrix = matrix4;
			const bool visible = (flags & 1) && node->visible();
			
			_tree->_flat_dag[node].push_back(DirectedAcyclicGraph::PerNodeInstance{
				.matrix = matrix,
				.visible = visible,
			});

			
			std::shared_ptr<Model> const& model = node->model();
			if (model)
			{
				std::shared_ptr<Mesh> const& mesh = model->mesh();
				std::shared_ptr<Material> const& material = model->material();
				uint32_t mesh_unique_id = -1;
				uint32_t material_unique_id = -1;
				uint32_t xform_unique_id = -1;
				if (mesh)
				{
					RigidMesh * rigid_mesh = dynamic_cast<RigidMesh*>(mesh.get());
					if (rigid_mesh)
					{
						if (!_unique_meshes.contains(mesh.get())) // unknown mesh so far
						{
							mesh_unique_id = _unique_mesh_index_pool.allocate();
							_unique_meshes[mesh.get()] = MeshData{
								.unique_index = mesh_unique_id,
							};

							rigid_mesh->registerToDescriptorSet(_set, _mesh_bindings_base, mesh_unique_id);
						}
						else
						{
							MeshData & md = _unique_meshes.at(mesh.get());
							mesh_unique_id = md.unique_index;
						}
					}
				}

				if(material)
				{
					PBMaterial * pb_material = dynamic_cast<PBMaterial*>(material.get());
					if (pb_material)
					{
						TextureAndSampler albedo_tex = pb_material->albedoTextureAndSampler();
						TextureAndSampler normal_tex = pb_material->normalTextureAndSampler();
						uint32_t albedo_texture_id = uint32_t(-1);
						uint32_t normal_texture_id = uint32_t(-1);
						if (albedo_tex.texture)
						{
							if (!_unique_textures.contains(albedo_tex.texture.get()))
							{
								albedo_texture_id = _unique_texture_2D_index_pool.allocate();
								albedo_tex.texture->registerToDescriptorSet(_set, _textures_bindings_base + 0, albedo_texture_id);
								// TODO 
								// With this impl, a sampler change will not propagate to the set
								// The should be managed inside of the material
								_set->setBinding(_textures_bindings_base + 0, albedo_texture_id, 1, nullptr, &albedo_tex.sampler);
								_unique_textures[albedo_tex.texture.get()] = TextureData{.unique_index = albedo_texture_id };
							}
							else
							{
								albedo_texture_id = _unique_textures.at(albedo_tex.texture.get()).unique_index;
							}
						}
						// TODO factorize this code with the above one
						if (normal_tex.texture)
						{
							if (!_unique_textures.contains(normal_tex.texture.get()))
							{
								normal_texture_id = _unique_texture_2D_index_pool.allocate();
								normal_tex.texture->registerToDescriptorSet(_set, _textures_bindings_base + 0, normal_texture_id);
								_set->setBinding(_textures_bindings_base + 0, normal_texture_id, 1, nullptr, &normal_tex.sampler);
								_unique_textures[normal_tex.texture.get()] = TextureData{ .unique_index = normal_texture_id };
							}
							else
							{
								normal_texture_id = _unique_textures.at(normal_tex.texture.get()).unique_index;
							}
						}

						bool set_material = false;

						if (!_unique_materials.contains(pb_material))
						{
							material_unique_id = _unique_material_index_pool.allocate();
							_unique_materials[pb_material] = MaterialData{.unique_index = material_unique_id};
							pb_material->registerToDescriptorSet(_set, _material_bindings_base + 0, material_unique_id, false);
							set_material = true;
						}
						else
						{
							material_unique_id = _unique_materials.at(pb_material).unique_index;
							const MaterialReference& mat_ref = _material_ref_buffer->get<MaterialReference>(material_unique_id);
							if (mat_ref.albedo_id != albedo_texture_id)
							{
								set_material = true;
							}
						}

						if (set_material)
						{
							MaterialReference mat_ref{
								.albedo_id = albedo_texture_id,
								.normal_id = normal_texture_id,
							};
							_material_ref_buffer->set<MaterialReference>(material_unique_id, mat_ref);
						}
					}
				}


				bool set_model_reference = false;
				bool set_xform = false;
				uint32_t unique_model_id;
				uint32_t model_flags = 0;
				Mat3x4 model_matrix = glm::transpose(matrix);
				if(visible)
					model_flags |= 1;
				if (!_unique_models.contains(path))
				{
					unique_model_id = _unique_model_index_pool.allocate();
					xform_unique_id = _unique_xform_index_pool.allocate();
					_unique_models[path] = ModelInstance{
						.model_unique_index = unique_model_id,
						.xform_unique_index = xform_unique_id,
					};
					set_model_reference = true;
					set_xform = true;
				}
				else
				{
					auto & um = _unique_models[path];
					unique_model_id = um.model_unique_index;
					xform_unique_id = um.xform_unique_index;
					const ModelReference & mr = _model_references_buffer->get<ModelReference>(unique_model_id);

					set_model_reference |= 
						(mr.flags != model_flags) || 
						(mr.mesh_id != mesh_unique_id) || 
						(mr.material_id || material_unique_id) || 
						(mr.xform_id != xform_unique_id);

					const Mat3x4 & registed_matrix = _xforms_buffer->get<Mat3x4>(xform_unique_id);
					set_xform |= (registed_matrix != model_matrix);
				}

				if (set_model_reference)
				{
					_model_references_buffer->set(unique_model_id, ModelReference{
						.mesh_id = mesh_unique_id,
						.material_id = material_unique_id,
						.xform_id = xform_unique_id,
						.flags = model_flags,
					});
				}
				
				if (set_xform)
				{
					_xforms_buffer->set<Mat3x4>(xform_unique_id, model_matrix);
				}
			}

			std::shared_ptr<Light> light = node->light();
			if (light)
			{
				LightInstanceData * lid = nullptr;
				if (_unique_light_instances.contains(path))
				{
					lid = &_unique_light_instances.at(path);
				}
				else
				{
					uint32_t light_unique_index = _unique_light_index_pool.allocate();
					lid = &_unique_light_instances[path];
					lid->unique_id = light_unique_index;
				}

				
				{
					if (light->enableShadowMap())
					{
						if (!lid->depth_view)
						{
							uint32_t layers = 1;
							VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
							VkImageCreateFlags flags = 0; 
							if (light->type() == LightType::POINT)
							{
								layers = 6;
								view_type = VK_IMAGE_VIEW_TYPE_CUBE;
								flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
							}
							std::shared_ptr<Image> depth_map_img = std::make_shared<Image>(Image::CI{
								.app = application(),
								.name = light->name() + ".depth_map",
								.flags = flags,
								.type = VK_IMAGE_TYPE_2D,
								.format = &_light_depth_format,
								.extent = VkExtent3D{.width = _light_resolion, .height = _light_resolion, .depth = 1,},
								.layers = layers,
								.samples = &_light_depth_samples,
								.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
								.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
							});
							lid->depth_view = std::make_shared<ImageView>(ImageView::CI{
								.app = application(),
								.name = light->name() + ".depth_map_view",
								.image = depth_map_img,
								.type = view_type,
							});

							if (layers == 1)
							{
								lid->depth_texture_unique_id = _light_depth_map_2D_index_pool.allocate();
								_set->setBinding(_lights_bindings_base + 1, lid->depth_texture_unique_id, 1, &lid->depth_view, nullptr);
							}
							else if (layers == 6)
							{
								lid->depth_texture_unique_id = _light_depth_map_cube_index_pool.allocate();
								_set->setBinding(_lights_bindings_base + 2, lid->depth_texture_unique_id, 1, &lid->depth_view, nullptr);
							}
						}
					}
				}



				if (visible)
				{
					LightGLSL gl = light->getAsGLSL(matrix);
					gl.textures.x = lid->depth_texture_unique_id;
					_lights_buffer->set(lid->unique_id, gl);
					++_num_lights;
				}
			}
			
			return true;
		});
	}

	void Scene::updateResources(UpdateContext& ctx)
	{
		//updateInternal();

		// Maybe separate between the few scene own internal resources and the lot of nodes resources (models, textures, ...)
		_ubo_buffer->updateResource(ctx);
		_lights_buffer->updateResources(ctx);
		_model_references_buffer->updateResources(ctx);
		_material_ref_buffer->updateResources(ctx);
		_xforms_buffer->updateResources(ctx);
		_prev_xforms_buffer->updateResource(ctx);
		


		_tree->iterateOnNodes([&](std::shared_ptr<Node> const& node)
		{
			node->updateResources(ctx);
		});

		if (_set_layout)
		{
			_set_layout->updateResources(ctx);
		}
		if (_set)
		{
			ctx.resourcesToUpdateLater() += _set;
			//_set->updateResources(ctx);
		}
		
		{
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = {
					PositionedObjectView{
						.obj = getUBO(),
						.pos = 0,
					},
				},
				.dst = _ubo_buffer->instance(),
			};
		}
	}

	void Scene::prepareForRendering(ExecutionRecorder& exec)
	{
		_xforms_buffer->recordTransferIFN(exec);
		_lights_buffer->recordTransferIFN(exec);
		_model_references_buffer->recordTransferIFN(exec);
		_material_ref_buffer->recordTransferIFN(exec);
	}
}