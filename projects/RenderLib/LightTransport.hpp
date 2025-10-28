#pragma once

#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/RayTracingCommand.hpp>

#include <vkl/Rendering/Scene.hpp>

#include <vkl/IO/ImGuiUtils.hpp>

namespace vkl
{
	class LightTransport : public Module
	{
	public:

		enum class Method
		{
			PathTracer,
			LightTracer,
			BidirectionalPathTracer,
		};

		struct UBO
		{
			u16 max_depth;
			u16 Li_resampling;
			uint flags;
		};

	protected:
		
		// Externally managed
		std::shared_ptr<Scene> _scene;
		std::shared_ptr<ImageView> _target;
		BufferAndRange _ubo;

		VkFormat _target_format = VK_FORMAT_UNDEFINED;
		std::string _target_format_str = {};
		bool _compile_time_max_depth = false;
		uint _max_depth = 5;
		uint _Li_resampling = 0;

		int _spectrum_mode = 0;
		std::string _spectrum_mode_str = {};

		MultiDescriptorSetsLayouts _sets_layouts;

		float _light_tracer_sample_mult = 1.0f;
		uint32_t _light_tracer_samples;
		std::shared_ptr<Buffer> _light_tracer_buffer = {};

		std::shared_ptr<Buffer> _bdpt_scratch_buffer = {};
		size_t _bdpt_scratch_buffer_segment_2 = 0;

		Method _method = Method::PathTracer;

		bool _use_rt_pipelines = false;

		std::shared_ptr<ComputeCommand> _path_tracer_rq;
		std::shared_ptr<ComputeCommand> _light_tracer_rq;
		std::shared_ptr<ComputeCommand> _bdpt_rq;

		std::shared_ptr<RayTracingCommand> _path_tracer_rt;
		std::shared_ptr<RayTracingCommand> _light_tracer_rt;
		std::shared_ptr<RayTracingCommand> _bdpt_rt;

		std::shared_ptr<ComputeCommand> _resolve_light_tracer;
		ImGuiListSelection _method_selection;

		void createInternals();

		bool useLightTracer() const;
		
	public:


		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<Scene> scene = nullptr;
			std::shared_ptr<ImageView> target = {};
			BufferAndRange ubo;
			MultiDescriptorSetsLayouts sets_layout = {};
		};
		using CI = CreateInfo;

		LightTransport(CreateInfo const& ci);

		virtual ~LightTransport() = default;

		void updateResources(UpdateContext& ctx);

		void render(ExecutionRecorder& exec);

		void declareGUI(GuiContext& ctx);

		void writeUBO(UBO& dst)
		{
			dst.max_depth = static_cast<u16>(_max_depth);
			dst.Li_resampling = static_cast<u16>(_Li_resampling);
			dst.flags = 0;
		}
	};
}