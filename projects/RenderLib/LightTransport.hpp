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

		};

	protected:
		
		// Externally managed
		std::shared_ptr<Scene> _scene;
		std::shared_ptr<ImageView> _target;
		BufferAndRange _ubo;

		MultiDescriptorSetsLayouts _sets_layouts;

		uint32_t _light_tracer_samples;
		std::shared_ptr<Buffer> _light_tracer_buffer = {};


		Method _method = Method::PathTracer;

		std::shared_ptr<ComputeCommand> _path_tracer_rq;
		std::shared_ptr<ComputeCommand> _light_tracer_rq;
		std::shared_ptr<ComputeCommand> _resolve_light_tracer;

		ImGuiListSelection _method_selection;

		void createInternals();
		
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
	};
}