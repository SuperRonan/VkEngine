#include "Scene.hpp"

#include <cassert>

#include <stack>

namespace vkl
{

	bool Scene::DirectedAcyclicGraph::checkIsAcyclic()const
	{
		// TODO
		return true;
	}

	void Scene::DirectedAcyclicGraph::iterateOnNodeThenSons(Node& node, Mat4 const& matrix, const PerNodeFunction& f)
	{
		Mat4 new_matrix = matrix * node.matrix4x4();
		f(matrix, node);
		for (std::shared_ptr<Node> const& n : node.children())
		{
			assert(!!n);
			iterateOnNodeThenSons(*n, new_matrix, f);
		}
	}

	void Scene::DirectedAcyclicGraph::iterate(const PerNodeFunction& f)
	{
		Mat4 matrix = Mat4(1);
		if (root())
		{
			iterateOnNodeThenSons(*root(), matrix, f);
		}
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

	void Scene::createSet()
	{
		std::shared_ptr<DescriptorSetLayout> layout = setLayout();

		ShaderBindings bindings;

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



	ResourcesToDeclare Scene::getResourcesToDeclare()
	{
		ResourcesToDeclare res;
		_tree->iterate([&res](Mat4 const& matrix, Node& node)
		{
			if (node.model())
			{
				res += node.model()->getResourcesToDeclare();
			}
		});
		res += _set;
		return res;
	}

	ResourcesToUpload Scene::getResourcesToUpload()
	{
		ResourcesToUpload res;

		_tree->iterate([&res](Mat4 const& matrix, Node& node)
		{
			if (node.model())
			{
				res += node.model()->getResourcesToUpload();
			}
		});

		return res;
	}

	void Scene::notifyDataIsUploaded()
	{
		_tree->iterate([](Mat4 const& matrix, Node& node)
		{
			if (node.model())
			{
				node.model()->notifyDataIsUploaded();
			}
		});
	}
}