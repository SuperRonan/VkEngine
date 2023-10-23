#pragma once

#include <Core/Execution/Executor.hpp>
#include <Core/Execution/Module.hpp>
#include <Core/Rendering/Scene.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>

#include <Core/Rendering/Model.hpp>
#include <Core/Rendering/RenderObjects.hpp>

namespace vkl
{
	class SimpleRenderer : public Module
	{
	protected:

		Executor& _exec;

		std::shared_ptr<Scene> _scene;
		std::shared_ptr<ImageView> _target;
		std::shared_ptr<ImageView> _depth;

		MultiDescriptorSetsLayouts _sets_layouts;

		std::shared_ptr<Buffer> _ubo_buffer = nullptr;

		struct UBO
		{
			float time;
			float delta_time;
			uint32_t frame_idx;

			alignas(16) glm::mat4 world_to_camera;
			alignas(16) glm::mat4 camera_to_proj;
			alignas(16) glm::mat4 world_to_proj;
		};
		
		std::shared_ptr<VertexCommand> _render_scene_direct = nullptr;
		struct RenderSceneDirectPC
		{
			glm::mat4 object_to_world;
		};
		std::shared_ptr<VertexCommand> _render_3D_basis = nullptr;

		std::shared_ptr<UpdateBuffer> _update_buffer = nullptr;

		std::vector<VertexCommand::DrawModelInfo> generateVertexDrawList();

		void createInternalResources();

	public:

		struct CreateInfo {
			VkApplication* app = nullptr;
			std::string name = {};
			Executor& exec;
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<Scene> scene = nullptr;
			std::shared_ptr<ImageView> target = nullptr;
		};
		using CI = CreateInfo;

		SimpleRenderer(CreateInfo const& ci);



		void execute(Camera const& camera, float time, float dt, uint32_t frame_id);

		std::shared_ptr<ImageView> depth() const
		{
			return _depth;
		}
	};
}