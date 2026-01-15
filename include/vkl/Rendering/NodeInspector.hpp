#pragma once

#include <vkl/GUI/Panel.hpp>
#include <vkl/Rendering/Scene.hpp>
#include <vkl/GUI/InlinePanel.hpp>

namespace vkl
{
	class SceneUserInterface;
}

namespace vkl::GUI
{
	class NodeInspector : public Panel
	{
	
	protected:

		SceneUserInterface* _parent = nullptr; // May be nullptr
		std::shared_ptr<Scene::Node> _node = {}; // ptr is Id
		bool _unique = false;

		TargetIndirectInlinePanel<Model> _model_panel;

	public:

		virtual void declareInline(Context& ctx) override;

		NodeInspector(SceneUserInterface* parent);

		NodeInspector(std::shared_ptr<Scene::Node> const& node, SceneUserInterface* parent);

		void setParent(SceneUserInterface* parent)
		{
			_parent = parent;
		}

		void reset(std::shared_ptr<Scene::Node> const& node);

		Id getDefaultId() const
		{
			return reinterpret_cast<Id>(_node.get());
		}

		void setUnique(bool unique);

		void resetName();

		const auto& node()const noexcept
		{
			return _node;
		}
	};
}