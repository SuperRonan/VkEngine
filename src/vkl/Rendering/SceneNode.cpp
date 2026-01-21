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
		if (_model)
		{
			_model->updateResources(ctx);
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