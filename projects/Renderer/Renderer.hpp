#pragma once

#include <Core/Execution/Executor.hpp>
#include <Core/Execution/Module.hpp>

#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>

#include <Core/Rendering/Model.hpp>
#include <Core/Rendering/RenderObjects.hpp>
#include <Core/Rendering/Scene.hpp>
#include <Core/Rendering/Camera.hpp>

#include <Core/IO/ImGuiUtils.hpp>
#include <Core/IO/GuiContext.hpp>

#include "AmbientOcclusion.hpp"

#include <unordered_map>
#include <map>

namespace vkl
{
	class SimpleRenderer : public Module
	{
	protected:

		std::shared_ptr<Scene> _scene = nullptr;
		std::shared_ptr<ImageView> _target = nullptr;
		std::shared_ptr<ImageView> _depth = nullptr;

		ImGuiListSelection _pipeline_selection = ImGuiListSelection::CI{
			.mode = ImGuiListSelection::Mode::RadioButtons,
			.labels = {"Direct V1"s, "Deferred V1"s},
			.default_index = 1,
			.same_line = true,
		};

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

		std::shared_ptr<AmbientOcclusion> _ambient_occlusion = nullptr;

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

		template <class DrawInfoType>
		using MultiDrawCallLists = std::map<uint32_t, DrawInfoType>;

		using MultiVertexDrawCallList = MultiDrawCallLists<VertexCommand::DrawInfo>;

		MultiVertexDrawCallList _cached_draw_list;
		void generateVertexDrawList(MultiVertexDrawCallList & res);

		void createInternalResources();

	public:

		struct CreateInfo {
			VkApplication* app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts;
			std::shared_ptr<Scene> scene = nullptr;
			std::shared_ptr<ImageView> target = nullptr;
		};
		using CI = CreateInfo;

		SimpleRenderer(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder& exec, Camera const& camera, float time, float dt, uint32_t frame_id);

		std::shared_ptr<ImageView> depth() const
		{
			return _depth;
		}

		void declareGui(GuiContext & ctx);
	};
}