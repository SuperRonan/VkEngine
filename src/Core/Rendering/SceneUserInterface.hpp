#pragma once

#include <Core/Rendering/Scene.hpp>
#include <Core/Rendering/Camera.hpp>

#include <Core/IO/GuiContext.hpp>
#include <Core/IO/ImGuiUtils.hpp>

#include <Core/Execution/Executor.hpp>

#include <Core/Commands/GraphicsCommand.hpp>

namespace vkl
{
	class SceneUserInterface : public VkObject
	{
	public:

		using Mat4 = Scene::Mat4;
		using Mat4x3 = Scene::Mat4x3;

		struct SelectedNode
		{
			Scene::DAG::PositionedNode node;
			Scene::DAG::FastNodePath path;

			bool hasValue()const
			{
				return node.node.operator bool();
			}

			void clear()
			{
				node.node = nullptr;
				node.matrix = Mat4(1);
				path.path.clear();
				bindMatrices();
			}

			void bindMatrices();

			ImGuiTransform3D gui_collapsed_matrix = {};
			ImGuiTransform3D gui_node_matrix = {};
		};

	protected:

		std::shared_ptr<Scene> _scene = nullptr;
		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;
		MultiDescriptorSetsLayouts _sets_layouts = {};
		

		std::shared_ptr<VertexCommand> _render_3D_basis = nullptr;
		struct Render3DBasisPC
		{
			Mat4 matrix;
		};

		std::shared_ptr<Mesh> _box_mesh = nullptr;
		std::shared_ptr<VertexCommand> _render_3D_box = nullptr;
		struct Render3DBoxPC
		{
			Mat4 matrix;
			glm::vec4 color;
		};

		SelectedNode _gui_selected_node;

		bool _show_world_basis = false;
		bool _show_view_basis = false;

		void createInternalResources();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<Scene> scene = nullptr;
			std::shared_ptr<ImageView> target = nullptr;
			std::shared_ptr<ImageView> depth = nullptr;
			MultiDescriptorSetsLayouts sets_layouts = {};
		};
		using CI = CreateInfo;

		SceneUserInterface(CreateInfo const& ci);

		virtual ~SceneUserInterface() override;

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & recorder, Camera & camera);

		virtual void declareGui(GuiContext & ctx);

		const SelectedNode& getGuiSelectedNode()const
		{
			return _gui_selected_node;
		}

		void checkSelectedNode(SelectedNode& selected_node);
	};
}