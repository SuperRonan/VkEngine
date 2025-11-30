#pragma once

#include <vkl/Execution/Executor.hpp>
#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/GraphicsCommand.hpp>
#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/TransferCommand.hpp>

#include <vkl/Rendering/Model.hpp>
#include <vkl/Rendering/RenderObjects.hpp>
#include <vkl/Rendering/Scene.hpp>
#include <vkl/Rendering/Camera.hpp>

#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Context.hpp>

#include "AmbientOcclusion.hpp"
#include "TemporalAntiAliasingAndUpscaler.hpp"
#include "DepthOfField.hpp"
#include "LightTransport.hpp"

#include <vkl/Commands/AccelerationStructureCommands.hpp>

#include <unordered_map>
#include <map>

namespace vkl
{
	class SimpleRenderer : public Module
	{
	public:

		enum class RenderPipeline
		{
			Forward = 0,
			Deferred = 1,
			LightTransport = 2,
		};

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
		bool _use_indirect_rendering = false;
		bool _use_fat_gbuffer = true;
		bool _use_reverse_depth = false;
		bool _maintain_rt = false;

		std::shared_ptr<Buffer> _draw_indexed_indirect_buffer = nullptr;
		BufferAndRange _vk_draw_params_segment;
		BufferAndRange _model_indices_segment;
		BufferAndRange _atomic_counter_segment;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		std::shared_ptr<ComputeCommand> _prepare_draw_list = nullptr;
		

		ImGuiListSelection _pipeline_selection;
		

		std::vector<uint32_t> _model_types;

		struct ForwardPipelineV1
		{
			std::shared_ptr<RenderPass> render_pass = nullptr;
			std::shared_ptr<Framebuffer> framebuffer = nullptr;
			std::map<uint32_t, std::shared_ptr<VertexCommand>> render_scene_direct;
			std::shared_ptr<VertexCommand> render_scene_indirect;
			struct RenderSceneDirectPC
			{
				Matrix4f object_to_world;
			};

			void updateResources(UpdateContext& ctx, bool update_direct = true, bool update_indirect = true);
		};
		ForwardPipelineV1 _forward_pipeline;

		struct DeferredPipelineBase
		{
			struct RasterCommands
			{
				std::shared_ptr<VertexCommand> raster;
			};
			std::shared_ptr<RenderPass> render_pass = nullptr;
			std::shared_ptr<Framebuffer> framebuffer = nullptr;

			std::map<uint32_t, RasterCommands> raster_gbuffer;
			std::shared_ptr<VertexCommand> raster_gbuffer_indirect;

			std::shared_ptr<ComputeCommand> shade_from_gbuffer = nullptr;

			void updateResources(UpdateContext& ctx, bool update_direct = true, bool update_indirect = true);
		};

		struct FatDeferredPipeline : public DeferredPipelineBase
		{
			struct RasterGBufferPC
			{
				Matrix4f object_to_world;
			};

			
			std::shared_ptr<ImageView> albedo = nullptr;
			std::shared_ptr<ImageView> position = nullptr;
			std::shared_ptr<ImageView> normal = nullptr;
			std::shared_ptr<ImageView> tangent = nullptr;

			
			
			void updateResources(UpdateContext& ctx, bool update_direct = true, bool update_indirect = true);
		};

		struct MinimalDeferredPipeline : public DeferredPipelineBase
		{
			struct RasterGBufferPC
			{
				Matrix4f object_to_world;
			};

			struct RasterCommands
			{
				std::shared_ptr<VertexCommand> raster;
			};
			std::shared_ptr<ImageView> ids = nullptr;
			// Triangle uv's 
			std::shared_ptr<ImageView> uvs = nullptr;

			void updateResources(UpdateContext& ctx, bool update_direct = true, bool update_indirect = true);
		};

		FatDeferredPipeline _fat_deferred_pipeline = {};
		MinimalDeferredPipeline _minimal_deferred_pipeline = {};

		std::shared_ptr<LightTransport> _light_transport;

		std::shared_ptr<TemporalAntiAliasingAndUpscaler> _taau;

		std::shared_ptr<Sampler> _light_depth_sampler = nullptr;
		std::shared_ptr<RenderPass> _spot_light_render_pass = nullptr;
		std::shared_ptr<VertexCommand> _render_spot_light_depth = nullptr;
		std::shared_ptr<RenderPass> _point_light_render_pass = nullptr;
		std::shared_ptr<VertexCommand> _render_point_light_depth = nullptr;

		std::shared_ptr<AmbientOcclusion> _ambient_occlusion = nullptr;
		std::shared_ptr<DepthOfField> _depth_of_field = nullptr;

		MultiDescriptorSetsLayouts _sets_layouts;

		std::shared_ptr<HostManagedBuffer> _ubo_buffer;
		BufferAndRange _ubo;
		struct UBO
		{
			float time;
			float delta_time;
			uint32_t frame_idx;

			alignas(16) Camera::AsGLSL camera;
		};

		const Camera* _camera = nullptr;

		template <class DrawInfoType>
		using MultiDrawCallLists = std::map<uint32_t, DrawInfoType>;

		using MultiVertexDrawCallList = MultiDrawCallLists<VertexCommand::DrawInfo>;

		MultiVertexDrawCallList _cached_draw_list;
		void generateVertexDrawList(MultiVertexDrawCallList & res);

		void createInternalResources();

		void updateMaintainRT();
		
		

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
			const Camera* camera = nullptr;
		};
		using CI = CreateInfo;

		SimpleRenderer(CreateInfo const& ci);

		void preUpdate(UpdateContext& ctx);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder& exec, float time, float dt, uint32_t frame_id);

		std::shared_ptr<ImageView> depth() const
		{
			return _depth;
		}

		void declareGui(GUI::Context & ctx);

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
			return _fat_deferred_pipeline.position;
		}

		std::shared_ptr<ImageView> const& getNormalImage()const
		{
			return _fat_deferred_pipeline.normal;
		}

		std::shared_ptr<ImageView> const& getAlbedoImage()const
		{
			return _fat_deferred_pipeline.albedo;
		}
	};
}