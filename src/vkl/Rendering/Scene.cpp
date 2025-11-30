#include <vkl/Rendering/Scene.hpp>

#include <cassert>
#include <stack>
#include <bitset>

#include <vkl/Utils/stl_extension.hpp>

#include <imgui/imgui.h>
#include <vkl/GUI/ImGuiUtils.hpp>

#include <vkl/Execution/Executor.hpp>

#include <vkl/Maths/Transforms.hpp>

#include <ShaderLib/Rendering/Scene/SceneFlags.h>

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

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFunction& f)
	{
		Mat3x4 node_matrix = node->matrix3x4();
		Mat3x4 new_matrix = matrix * node_matrix;
		bool visible = node->visible();
		std::bitset<32> flags_bits(flags);
		flags_bits.set(0, visible && flags_bits[0]);
		uint32_t new_flags = flags_bits.to_ulong();
		if (f(node, new_matrix, new_flags))
		{
			for (std::shared_ptr<Node> const& n : node->children())
			{
				assert(!!n);
				iterateOnNodeThenSons(n, new_matrix, new_flags, f);
			}
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, FastNodePath & path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFastPathFunction& f)
	{
		Mat3x4 node_matrix = node->matrix3x4();
		Mat3x4 new_matrix = matrix * node_matrix;
		bool visible = node->visible();
		std::bitset<32> flags_bits(flags);
		flags_bits.set(0, visible && flags_bits[0]);
		uint32_t new_flags = flags_bits.to_ulong();
		if (f(node, path, new_matrix, new_flags))
		{
			path.path.push_back(0);
			for (size_t i=0; i < node->children().size(); ++i)
			{
				path.path.back() = static_cast<uint32_t>(i);
				std::shared_ptr<Node> const& n = node->children()[i];
				assert(!!n);
				iterateOnNodeThenSons(n, path, new_matrix, new_flags, f);
			}
			path.path.pop_back();
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, RobustNodePath& path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceRobustPathFunction& f)
	{
		Mat3x4 node_matrix = node->matrix3x4();
		Mat3x4 new_matrix = matrix * node_matrix;
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
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), matrix, 1, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceFastPathFunction & f)
	{
		FastNodePath path;
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, 1, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceRobustPathFunction & f)
	{
		RobustNodePath path;
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, 1, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeInstanceFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			for (const auto& instance : instances)
			{
				f(node, instance.matrix, instance.flags);
			}
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeAllInstancesFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			f(node, instances);
		}
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodes(const PerNodeFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			f(node);
		}
	}

	void Scene::DirectedAcyclicGraph::flatten()
	{
		_flat_dag.clear();

		const auto process_node = [&](std::shared_ptr<Node> const& node, Mat3x4 const& matrix, uint32_t flags)
		{
			std::vector<PerNodeInstance> & matrices = _flat_dag[node];
			matrices.push_back(PerNodeInstance{
				.matrix = matrix, 
				.flags = flags,
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
		Mat3x4 matrix = n->matrix3x4();
		for (size_t i = 0; i < path.path.size(); ++i)
		{
			if (path.path[i] < n->children().size())
			{
				n = n->children()[path.path[i]];
				matrix = matrix * (n->matrix4x4());
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
		Mat3x4 matrix = n->matrix3x4();
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
				matrix *= n->matrix3x4();
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

		if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_maintain_rt = true;
		}

		createSet();
	}

	Scene::UBO Scene::getUBO()const
	{
		uint32_t flags = 0;
		float solar_intensity_norm = _solar_disk_angle == 0 ? 1 : sqr(_solar_disk_angle);
		vec3 solar_disk_emission = Light::NormalizeEmission(_solar_disk_emission, _solar_disk_emission_options, solar_intensity_norm);
		if (_solar_disk_emission_options & EMISSION_FLAG_BLACK_BODY_BIT)
		{
			flags |= SCENE_FLAG_SOLAR_BLACK_BODY_EMISSION_BIT;
		}
		UBO res{
			.num_lights = _num_lights,
			.flags = flags,
			.ambient = _ambient,
			.sky = _uniform_sky * _uniform_sky_brightness,
			
			.solar_direction = SphericalToCartesian(_solar_disk_direction),
			.solar_disk_cosine = std::cos(_solar_disk_angle),
			.solar_disk_emission = solar_disk_emission,
			.solar_disk_angle = _solar_disk_angle,
			
			.center = _aabb.center(),
			.radius = _radius,
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
			_tlas_binding_base = _xforms_bindings_base + xforms_num_bindings;
			const uint32_t tlas_num_bindings = 1;

			MyVector<DescriptorSetLayout::Binding> bindings;
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


			if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
			{
				bindings += DescriptorSetLayout::Binding{
					.name = "SceneTLAS",
					.binding = _tlas_binding_base,
					.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
					.count = 1,
					.stages = VK_SHADER_STAGE_ALL,
					.access = 0,
					.usage = 0,
				};
			}

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
			.size = sizeof(Mat3x4) * 256,
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

		if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			_tlas = std::make_shared<TopLevelAccelerationStructure>(TopLevelAccelerationStructure::CI{
				.app = application(),
				.name = name() + ".TLAS",
				.geometry_flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
				.geometries = {
					TLAS::GeometryCreateInfo{
						.flags = 0,
						.capacity = 16,
					},
				},
				.build_flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
				.hold_instance = &_maintain_rt,
			});
			_build_tlas = std::make_shared<BuildAccelerationStructureCommand>(BuildAccelerationStructureCommand::CI{
				.app = application(),
				.name = name() + ".BuildTLAS",
			});
		}
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
			.buffer = _xforms_segment,
			.binding = _xforms_bindings_base + 0,
		};

		bindings += Binding{
			.buffer = _prev_xforms_segment,
			.binding = _xforms_bindings_base + 1,
		};

		if (application()->availableFeatures().acceleration_structure_khr.accelerationStructure)
		{
			bindings += Binding{
				.tlas = _tlas,
				.binding = _tlas_binding_base + 0,
			};
		}

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
		
		_tree->iterateOnFlattenDag([&](std::shared_ptr<Node> const& node, Mat3x4 const& matrix, uint32_t flags)
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
		
		_aabb.reset();

		_tree->iterateOnDag([&](std::shared_ptr<Node> const& node, DirectedAcyclicGraph::RobustNodePath const& path, Mat3x4 const& matrix4, uint32_t flags)
		{
			const Mat3x4 matrix = matrix4;
			const bool visible = (flags & 1) && node->visible();
			
			_tree->_flat_dag[node].push_back(DirectedAcyclicGraph::PerNodeInstance{
				.matrix = matrix,
				.flags = flags,
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

					const auto & aabb = mesh->getAABB();

					aabb.getContainingAABB(matrix, _aabb);
				}

				if(material)
				{
					bool write_material = false;
					if (!_unique_materials.contains(material.get()))
					{
						material_unique_id = _unique_material_index_pool.allocate();
						_unique_materials[material.get()] = MaterialData{
							.unique_index = material_unique_id,
						};
						material->registerToDescriptorSet(_set, _material_bindings_base + 0, material_unique_id, false);
						write_material = true;
					}
					else
					{
						material_unique_id = _unique_materials[material.get()].unique_index;
					}
					const MaterialReference& old_ref = _material_ref_buffer->get<MaterialReference>(material_unique_id);


					MaterialReference mat_ref = {};
					bool write_mat_ref = false;
					for (uint i = 0; i < Material::MAX_TEXTURE_COUNT; ++i)
					{
						uint16_t & id = mat_ref.ids[i];
						const TextureAndSampler tas = material->getTextureAndSampler(i);
						if (tas.texture)
						{
							if (!_unique_textures.contains(tas.texture.get()))
							{
								id = _unique_texture_2D_index_pool.allocate();
								tas.texture->registerToDescriptorSet(_set, _textures_bindings_base + 0, id);
								_unique_textures[tas.texture.get()] = TextureData{
									.unique_index = static_cast<uint32_t>(id),
								};
								// TODO register the sample change too
								_set->setBinding(_textures_bindings_base + 0, id, 1, nullptr, &tas.sampler);
							}
							else
							{
								id = static_cast<uint16_t>(_unique_textures.at(tas.texture.get()).unique_index);
							}
							if (old_ref.ids[i] != id)
							{
								write_mat_ref |= true;
							}
						}
					}

					if (write_mat_ref)
					{
						_material_ref_buffer->set<MaterialReference>(material_unique_id, mat_ref);
					}
				}


				bool set_model_reference = false;
				bool set_xform = false;
				bool changed_xform = false;
				uint32_t unique_model_id;
				uint32_t model_flags = 0;
				Mat3x4 model_matrix = matrix;
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

					const Mat3x4& registed_matrix = _xforms_buffer->get<Mat3x4>(xform_unique_id);
					changed_xform = (registed_matrix != model_matrix);
					set_xform |= changed_xform;
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
				
				if (_maintain_rt)
				{
					Mesh * mesh = model->mesh().get();
					const uint32_t tlas_geometry_id = 0;
					if (mesh)
					{
						const bool should_be_registered_to_tlas = ((flags & 0x1) != 0) && mesh->isReadyToDraw();
						const bool is_already_registered_to_tlas = (unique_model_id < _tlas->geometries()[tlas_geometry_id].blases.size()) && (_tlas->geometries()[tlas_geometry_id].blases[unique_model_id].blas == mesh->blas());
						if (should_be_registered_to_tlas)
						{
							VkGeometryInstanceFlagsKHR geometry_flags = 0;
							if (material->isOpaque())
							{
								geometry_flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
							}
							else
							{
								VKL_BREAKPOINT_HANDLE;
							}
							bool register_to_tlas = !is_already_registered_to_tlas;
							if (register_to_tlas)
							{
								_tlas->registerBLAS(tlas_geometry_id, unique_model_id, TLAS::BLASInstance{
									.blas = mesh->blas(),
									.xform = ConvertXFormToVk(matrix),
									.instanceCustomIndex = unique_model_id,
									.mask = 0xFF,
									.instanceShaderBindingTableRecordOffset = 0,
									.flags = geometry_flags,
								});
							}
							else if (changed_xform)
							{
								_tlas->geometries()[tlas_geometry_id].blases[unique_model_id].setXForm(matrix);
							}
							if (!register_to_tlas)
							{
								_tlas->geometries()[tlas_geometry_id].blases[unique_model_id].setFlagsIFN(geometry_flags);
							}
						}
						else if(!should_be_registered_to_tlas && is_already_registered_to_tlas)
						{
							_tlas->registerBLAS(tlas_geometry_id, unique_model_id, TLAS::BLASInstance{
								.blas = nullptr,
							});
						}

					}
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
				lid->flags = flags;
				lid->frame_light_id = _num_lights;
				
				{
					if (light->enableShadowMap())
					{
						if (!lid->depth_view)
						{
							uint32_t layers = 1;
							VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
							VkImageCreateFlags flags = 0; 
							if (light->type() == LightType::Point)
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
								.extent = [this](){return VkExtent3D{.width = _light_resolution, .height = _light_resolution, .depth = 1,}; },
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
					gl.textures[0] = lid->depth_texture_unique_id;
					_lights_buffer->set(lid->frame_light_id, gl);
					++_num_lights;
				}
			}
			
			return true;
		});

		_radius = _aabb.getContainingSphere().radius();
	}

	void Scene::setMaintainRT(bool value)
	{
		_maintain_rt = value;
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

		if (_tlas)
		{
			_tlas->updateResources(ctx);
		}

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
			const UBO ubo = getUBO();
			ResourcesToUpload::BufferSource src{
				.data = &ubo,
				.size = sizeof(ubo),
				.copy_data = true,
			};
			ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
				.sources = &src,
				.sources_count = 1,
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

	void Scene::buildTLAS(ExecutionRecorder& exec)
	{
		if (_tlas)
		{
			_tlas->recordTransferIFN(exec);
			_tlas_build_info.pushIFN(_tlas);
			if (!_tlas_build_info.empty())
			{
				exec(_build_tlas->with(_tlas_build_info));
			}
			_tlas_build_info.clear();
		}
	}
}