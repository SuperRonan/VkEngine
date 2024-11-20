#pragma once

#include <vkl/Rendering/Scene.hpp>
#include <vkl/Rendering/Camera.hpp>

#include <vkl/IO/GuiContext.hpp>
#include <vkl/IO/ImGuiUtils.hpp>

#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/GraphicsCommand.hpp>

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
			}
		};

	protected:

		std::shared_ptr<Scene> _scene = nullptr;
		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;
		MultiDescriptorSetsLayouts _sets_layouts = {};
		
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;

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

		class CreateNodePopUp
		{
		protected:

			ImVec2 _center;
			ImGuiPopupFlags _flags = ImGuiWindowFlags_AlwaysAutoResize * 0;

			std::string _path_str = {};
			std::filesystem::path _path = {};
			bool _synch = false;
			bool _popup_open = true;
			bool _file_dialog_open = false;

			std::mutex _file_dialog_mutex;

			//friend void FileDialogCallback(void* p_user_data, const char* const* file_list, int filter);

		public:

			void open();

			void openFileDialog();

			void close();

			// Returns:
			// -1 on close
			// 0 on pending
			// 1 on finish
			int declareGUI(GuiContext& ctx);

			std::shared_ptr<Scene::Node> createNode(VkApplication * app);

			bool canCreateNodeFromFile() const;

			std::string_view name() const
			{
				return "Node Creating"sv;
			}

			ImGuiPopupFlags flags()const
			{
				return _flags;
			}
		};

		CreateNodePopUp _create_node_popup = {};

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