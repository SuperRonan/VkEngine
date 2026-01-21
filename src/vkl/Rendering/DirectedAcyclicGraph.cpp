#include <vkl/Rendering/DirectedAcyclicGraph.hpp>

#include <bitset>

namespace vkl
{
	using DAG = DirectedAcyclicGraph;
	static_assert(std::convertible_to<DAG::FastNodePathView, DAG::FastNodePath>);
	static_assert(std::convertible_to<DAG::FastNodePath, DAG::FastNodePathView>);

	bool DirectedAcyclicGraph::checkIsAcyclic()const
	{
		// TODO
		return true;
	}

	void DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFunction& f)
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

	void DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, FastNodePath& path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFastPathFunction& f)
	{
		Mat3x4 node_matrix = node->matrix3x4();
		Mat3x4 new_matrix = matrix * node_matrix;
		bool visible = node->visible();
		std::bitset<32> flags_bits(flags);
		flags_bits.set(0, visible && flags_bits[0]);
		uint32_t new_flags = flags_bits.to_ulong();
		if (f(node, path, new_matrix, new_flags))
		{
			path.push_back(0);
			for (size_t i = 0; i < node->children().size(); ++i)
			{
				path.back() = static_cast<uint32_t>(i);
				std::shared_ptr<Node> const& n = node->children()[i];
				assert(!!n);
				iterateOnNodeThenSons(n, path, new_matrix, new_flags, f);
			}
			path.pop_back();
		}
	}

	void DirectedAcyclicGraph::iterateOnNodeThenSons(std::shared_ptr<Node> const& node, RobustNodePath& path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceRobustPathFunction& f)
	{
		Mat3x4 node_matrix = node->matrix3x4();
		Mat3x4 new_matrix = matrix * node_matrix;
		bool visible = node->visible();
		std::bitset<32> flags_bits(flags);
		flags_bits.set(0, visible && flags_bits[0]);
		uint32_t new_flags = flags_bits.to_ulong();
		if (f(node, path, new_matrix, new_flags))
		{
			path.push_back(nullptr);

			for (size_t i = 0; i < node->children().size(); ++i)
			{
				std::shared_ptr<Node> const& n = node->children()[i];
				assert(!!n);
				path.back() = n.get();
				iterateOnNodeThenSons(n, path, new_matrix, new_flags, f);
			}
			path.pop_back();
		}
	}

	void DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceFunction& f)
	{
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), matrix, 1, f);
		}
	}

	void DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceFastPathFunction& f)
	{
		FastNodePath path;
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, 1, f);
		}
	}

	void DirectedAcyclicGraph::iterateOnDag(const PerNodeInstanceRobustPathFunction& f)
	{
		RobustNodePath path;
		Mat3x4 matrix = Mat3x4::Identity();
		if (root())
		{
			iterateOnNodeThenSons(root(), path, matrix, 1, f);
		}
	}

	void DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeInstanceFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			for (const auto& instance : instances)
			{
				f(node, instance.matrix, instance.flags);
			}
		}
	}

	void DirectedAcyclicGraph::iterateOnFlattenDag(const PerNodeAllInstancesFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			f(node, instances);
		}
	}

	void DirectedAcyclicGraph::iterateOnNodes(const PerNodeFunction& f)
	{
		for (auto& [node, instances] : _flat_dag)
		{
			f(node);
		}
	}

	std::span<const DirectedAcyclicGraph::PerNodeInstance> DirectedAcyclicGraph::getNodeInstancesView(std::shared_ptr<Node> const& node) const
	{
		std::span<const DirectedAcyclicGraph::PerNodeInstance> res = {};
		auto it = _flat_dag.find(node);
		if (it != _flat_dag.end())
		{
			res = std::span<const DirectedAcyclicGraph::PerNodeInstance>(it->second);
		}
		return res;
	}

	void DirectedAcyclicGraph::flatten()
	{
		_flat_dag.clear();
		_flat_path_storage.clear();

		const auto process_node = [&](std::shared_ptr<Node> const& node, FastNodePathView path, Mat3x4 const& matrix, uint32_t flags)
			{
				std::vector<PerNodeInstance>& matrices = _flat_dag[node];
				size_t path_index = _flat_path_storage.pushBack(path.data(), path.size());
				matrices.push_back(PerNodeInstance{
					.matrix = matrix,
					.flags = flags,
					.fast_path_ref = Range32u{.begin = u32(path_index), .len = u32(path.size())},
					});
				return true;
			};

		iterateOnDag(process_node);
	}

	DirectedAcyclicGraph::DirectedAcyclicGraph(std::shared_ptr<Node> root) :
		_root(std::move(root))
	{

	}

	DirectedAcyclicGraph::PositionedNode DirectedAcyclicGraph::findNode(FastNodePathView const& path)
	{
		PositionedNode res;

		std::shared_ptr<Node> n = _root;
		Mat3x4 matrix = n->matrix3x4();
		for (size_t i = 0; i < path.size(); ++i)
		{
			if (path[i] < n->children().size())
			{
				n = n->children()[path[i]];
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

	DirectedAcyclicGraph::PositionedNode DirectedAcyclicGraph::findNode(RobustNodePathView const& path)
	{
		PositionedNode res;
		std::shared_ptr<Node> n = _root;
		Mat3x4 matrix = n->matrix3x4();
		for (size_t i = 0; i < path.size(); ++i)
		{
			auto it = std::ranges::find_if(n->children(), [&](auto const& it) {return it.get() == path[i]; });
			if (it != n->children().end())
			{
				n = *it;
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

	DirectedAcyclicGraph::FastNodePath DirectedAcyclicGraph::getFastPath(RobustNodePathView const& path)
	{
		FastNodePath res = {};
		res.resize(path.size());
		const Node* n = _root.get();
		for (size_t i = 0; i < path.size(); ++i)
		{
			auto it = std::ranges::find_if(n->children(), [&](auto const& it) {return it.get() == path[i]; });
			if (it != n->children().end())
			{
				res[i] = (it - n->children().begin());
				n = it->get();
			}
			else
			{
				res.clear();
				break;
			}
		}
		RobustNodePath tmp = RobustNodePath(path);
		return res;
	}

	DirectedAcyclicGraph::RobustNodePath DirectedAcyclicGraph::getRobustPath(FastNodePathView const& path)
	{
		RobustNodePath res = {};
		res.resize(path.size());
		Node* n = _root.get();
		for (size_t i = 0; i < path.size(); ++i)
		{
			const uint32_t index = path[i];
			if (index < n->children().size())
			{
				n = n->children()[i].get();
				res[i] = n;
			}
			else
			{
				res.clear();
				break;
			}
		}
		return res;
	}
}