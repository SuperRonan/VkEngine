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

		std::shared_ptr<Scene::Node> _node = {}; // ptr is Id
		SceneUserInterface* _parent = nullptr; // May be nullptr
		bool _unique = false;

		TargetIndirectInlinePanel<Model> _model_panel;

	public:

		NodeInspector(std::shared_ptr<Scene::Node> const& node);

		virtual void declareInline(Context& ctx) override;

		void setParent(SceneUserInterface* parent)
		{
			_parent = parent;
		}

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