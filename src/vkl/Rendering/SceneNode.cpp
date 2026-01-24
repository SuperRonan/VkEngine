#include <vkl/Rendering/SceneNode.hpp>

#include <vkl/Rendering/NodeInspector.hpp>

namespace vkl
{
	SceneNode::SceneNode(CreateInfo const& ci) :
		VkObject(ci.app, ci.name),
		_matrix(ci.matrix),
		_model(ci.model)
	{
		if (!application())
		{
			VKL_BREAKPOINT_HANDLE;
		}
	}

	void SceneNode::updateResources(UpdateContext& ctx)
	{
		if (ctx.updateTick() <= _latest_update_tick)
		{
			return;
		}
		_latest_update_tick = ctx.updateTick();
		_aabb.clear();
		if (_model)
		{
			_model->updateResources(ctx);
			if (auto& mesh = _model->mesh())
			{
				_aabb += mesh->getAABB();
			}
		}
		for (auto& child : _children)
		{
			child->updateResources(ctx);
			child->_aabb.getContainingAABB(child->getXForm(), _aabb);
		}
	}

	std::shared_ptr<GUI::Panel> SceneNode::makeInspector(std::shared_ptr<Scene::Node> const& shared_this, GUI::Context& ctx)
	{
		return std::make_shared<GUI::NodeInspector>(shared_this);
	}

	void SceneNode::collapseAuxiliaryTransform()
	{
		_matrix = _matrix * getAuxiliaryTransform();
		resetAuxiliaryTransform();
	}
}