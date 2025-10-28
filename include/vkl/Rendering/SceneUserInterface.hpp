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

		using Vec4 = Scene::Vec4;
		using Mat3x4 = Scene::Mat3x4;
		using Mat4 = Scene::Mat4;

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
				node.matrix = Mat3x4::Identity();
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
			Vec4 color;
		};

		SelectedNode _gui_selected_node;

		class CreateNodePopUp
		{
		protected:

			// 0 : Empty node
			// 1 : Node from file
			// 2 : Mesh + Material node
			// 3 : Light
			uint _type = 0;
			ImGuiPopupFlags _flags = ImGuiWindowFlags_AlwaysAutoResize * 0;
			
			std::string _path_str = {};
			std::filesystem::path _path = {};
			bool _synch = false;
			bool _popup_open = true;
			bool _file_dialog_open = false;

			std::mutex _file_dialog_mutex;

			Vector3f _color = {};
			uint _sub_type = 0;
			Vector2u _subdivisions = {16, 32};

			float _float_1 = 0, _float_2 = 0, _float_3 = 0;
			bool _bool_1 = false, _bool_2 = false;


			std::shared_ptr<Scene::Node> _parent = {};
			//friend void FileDialogCallback(void* p_user_data, const char* const* file_list, int filter);

		public:

			void open(std::shared_ptr<Scene::Node> const& parent);

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

			auto getParent()const
			{
				return _parent;
			}

			void resetParent()
			{
				_parent.reset();
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