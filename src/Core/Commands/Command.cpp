
#include "Command.hpp"
#include <cassert>

namespace vkl
{
	void ExecutionNode::clear()
	{
		_resources.clear();
	}
	

	void ExecutionNodePool::recycleNodes()
	{
		// Assume mutex is locked
		while (!_in_use_nodes.empty())
		{
			std::shared_ptr<ExecutionNode> & n = _in_use_nodes.front();
			if (n->isInUse())
			{
				break;
			}
			else
			{
				_available_nodes.push_back(std::move(n));
				_in_use_nodes.pop_front();
			}
		}
	}

}