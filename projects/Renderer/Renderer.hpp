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
#include "TemporalAntiAliasingAndUpscaler.hpp"

#include <Core/Commands/AccelerationStructureCommands.hpp>

#include <unordered_map>
#include <map>

namespace vkl
{
	class SimpleRenderer : public Module
	{
	public:

		enum class ShadowMethod
		{
			None = 0,
			ShadowMap = 1,
			RayTraced = 2,
		};

	protected:

		std::shared_ptr<Scene> _scene = nullptr;
		
		// "Read only"
		std::shared_ptr<ImageView> _output_target = nullptr;

		std::shared_ptr<ImageView> _render_target = nullptr;
		
		std::shared_ptr<ImageView> _depth = nullptr;

		uint32_t _model_capacity = 256;
		std::shared_ptr<Buffer> _draw_indexed_indirect_buffer = nullptr;
		BufferAndRange _vk_draw_params_segment;
		BufferAndRange _model_indices_segment;
		BufferAndRange _atomic_counter_segment;

		std::shared_ptr<ComputeCommand> _prepare_draw_list = nullptr;
		

		ImGuiListSelection _pipeline_selection;
		bool _use_indirect_rendering = false;

		std::vector<uint32_t> _model_types;

		struct DirectPipelineV1
		{
			std::map<uint32_t, std::shared_ptr<VertexCommand>> _render_scene_direct;
			std::shared_ptr<VertexCommand> _render_scene_indirect;
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
			std::shared_ptr<ImageView> _tangent = nullptr;

			struct RasterCommands
			{
				std::shared_ptr<VertexCommand> raster;
			};
			std::shared_ptr<VertexCommand> raster_gbuffer_indirect;
			std::map<uint32_t, RasterCommands> _raster_gbuffer;
			struct RasterGBufferPC 
			{
				glm::mat4 object_to_world;
			};
			std::shared_ptr<ComputeCommand> _shade_from_gbuffer = nullptr;
		};
		DeferredPipelineV1 _deferred_pipeline;

		struct PathTracer
		{
			std::shared_ptr<ComputeCommand> _path_trace;

			struct UBO
			{
				
			};
			BufferAndRange _ubo;
		};
		PathTracer _path_tracer;

		std::shared_ptr<TemporalAntiAliasingAndUpscaler> _taau;

		std::shared_ptr<Sampler> _light_depth_sampler = nullptr;
		std::shared_ptr<VertexCommand> _render_spot_light_depth = nullptr;
		std::shared_ptr<VertexCommand> _render_point_light_depth = nullptr;

		std::shared_ptr<AmbientOcclusion> _ambient_occlusion = nullptr;

		MultiDescriptorSetsLayouts _sets_layouts;

		std::shared_ptr<HostManagedBuffer> _ubo_buffer;
		BufferAndRange _ubo;
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

		bool _maintain_rt = false;

		std::shared_ptr<BuildAccelerationStructureCommand> _build_as = nullptr;
		BuildAccelerationStructureCommand::BuildInfo _blas_build_list;

		ImGuiListSelection _shadow_method;
		std::string _shadow_method_glsl_def;
		std::string _use_ao_glsl_def;

		void FillASBuildLists();


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

		std::shared_ptr<ImageView> const& renderTarget() const
		{
			return _render_target;
		}

		std::shared_ptr<ImageView> const& output()const
		{
			return _taau->output();
		}

		std::shared_ptr<ImageView> getAmbientOcclusionTargetIFP() const
		{
			std::shared_ptr<ImageView> res = nullptr;
			if (_ambient_occlusion)
			{
				res = _ambient_occlusion->target();
			}
			return res;
		}

		std::shared_ptr<ImageView> const& getPositionImage() const
		{
			return _deferred_pipeline._position;
		}

		std::shared_ptr<ImageView> const& getNormalImage()const
		{
			return _deferred_pipeline._normal;
		}

		std::shared_ptr<ImageView> const& getAlbedoImage()const
		{
			return _deferred_pipeline._albedo;
		}
	};
}