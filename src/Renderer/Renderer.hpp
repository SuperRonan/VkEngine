#pragma once

#include <Core/Execution/Executor.hpp>
#include <Core/Execution/Module.hpp>
#include <Core/Rendering/Scene.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>

#include <Core/Rendering/Model.hpp>
#include <Core/Rendering/RenderObjects.hpp>

#include <Core/IO/ImGuiUtils.hpp>

#include <unordered_map>
#include <map>

namespace vkl
{
	class SimpleRenderer : public Module
	{
	protected:

		Executor& _exec;

		std::shared_ptr<Scene> _scene = nullptr;
		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;

		ImGuiRadioButtons _pipeline_selection = std::vector<std::string>{"Direct V1"s, "Deferred V1"s};

		std::vector<uint32_t> _model_types;

		struct DirectPipelineV1
		{
			std::map<uint32_t, std::shared_ptr<VertexCommand>> _render_scene_direct;
			struct RenderSceneDirectPC
			{
				glm::mat4 object_to_world;
			};
		};
		DirectPipelineV1 _direct_pipeline;

		struct DeferredPipelineV1
		{
			// GBuffer layers
			std::shared_ptr<ImageView> _albedo = nullptr;
			std::shared_ptr<ImageView> _position = nullptr;
			std::shared_ptr<ImageView> _normal = nullptr;

			std::map<uint32_t, std::shared_ptr<VertexCommand>> _raster_gbuffer;
			struct RasterGBufferPC 
			{
				glm::mat4 object_to_world;
			};
			std::shared_ptr<ComputeCommand> _shade_from_gbuffer = nullptr;
		};
		DeferredPipelineV1 _deferred_pipeline;

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
		
		
		std::shared_ptr<VertexCommand> _render_3D_basis = nullptr;

		std::shared_ptr<UpdateBuffer> _update_buffer = nullptr;


		bool _show_world_3D_basis = false;
		bool _show_view_3D_basis = false;

		template <class DrawCallType>
		using MultiDrawCallLists = std::map<uint32_t, std::vector<DrawCallType>>;

		using MultiVertexDrawCallList = MultiDrawCallLists<VertexCommand::DrawCallInfo>;

		MultiVertexDrawCallList generateVertexDrawList();

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



		void execute(ExecutionThread& exec, Camera const& camera, float time, float dt, uint32_t frame_id);

		std::shared_ptr<ImageView> depth() const
		{
			return _depth;
		}

		void declareImGui();
	};
}