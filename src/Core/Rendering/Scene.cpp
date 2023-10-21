#include "Scene.hpp"

#include <cassert>

#include <stack>
#include <Core/Utils/stl_extension.hpp>

namespace vkl
{

	bool Scene::DirectedAcyclicGraph::checkIsAcyclic()const
	{
		// TODO
		return true;
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat4 const& matrix, const PerNodeInstanceFunction& f)
	{
		Mat4 new_matrix = matrix * node->matrix4x4();
		f(node, new_matrix);
		for (std::shared_ptr<Node> const& n : node->children())
		{
			assert(!!n);
			iterateOnNodeThenSons(n, new_matrix, f);
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
		};

		iterateOnDag(process_node);
	}

	Scene::DirectedAcyclicGraph::DirectedAcyclicGraph(std::shared_ptr<Node> root):
		_root(std::move(root))
	{
		
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
		});
	}


	ResourcesToDeclare Scene::getResourcesToDeclare()
	{
		ResourcesToDeclare res;
		_tree->iterateOnNodes([&res](std::shared_ptr<Node> const& node)
		{
			if (node->model())
			{
				res += node->model()->getResourcesToDeclare();
			}
		});

		res += _ubo_buffer;
		res += _lights_buffer;

		res += _set;
		return res;
	}

	ResourcesToUpload Scene::getResourcesToUpload()
	{
		ResourcesToUpload res;

		_tree->iterateOnNodes([&res](std::shared_ptr<Node> const& node)
		{
			if (node->model())
			{
				res += node->model()->getResourcesToUpload();
			}
		});

		res += ResourcesToUpload::BufferUpload{
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
			res += ResourcesToUpload::BufferUpload{
				.sources = {
					PositionedObjectView{
						.obj = _lights_glsl,
						.pos = 0,
					},
				},
				.dst = _lights_buffer,
			};
		}

		return res;
	}

	void Scene::notifyDataIsUploaded()
	{
		_tree->iterateOnNodes([](std::shared_ptr<Node> const& node)
		{
			if (node->model())
			{
				node->model()->notifyDataIsUploaded();
			}
		});
	}


	void Scene::prepareForRendering()
	{
		_tree->flatten();
		fillLightsBuffer();
	}
}