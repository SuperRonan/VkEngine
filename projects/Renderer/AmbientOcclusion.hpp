#pragma once

#include <Core/Execution/Module.hpp>
#include <Core/Execution/Executor.hpp>

#include <Core/Commands/ComputeCommand.hpp>

#include <Core/IO/GuiContext.hpp>
#include <Core/IO/ImGuiUtils.hpp>

#include <Core/Rendering/Camera.hpp>


namespace vkl
{
	class AmbientOcclusion : public Module
	{
	public:

		enum class Method
		{
			SSAO = 0,
			RQAO = 1,
		};
		
		struct UBO
		{
			float radius;
		};

	protected:

		bool _can_rt = false;
		bool _enable = true;
		int _ao_samples = 16;
		float _radius = 0.02;
		int _downscale = 2;
		uint32_t _seed = 0;

		ImGuiListSelection _gui_method;

		MultiDescriptorSetsLayouts _sets_layouts = {};

		std::string _method_glsl;

		std::shared_ptr<ComputeCommand> _command = nullptr;

		struct CommandPC
		{
			glm::vec3 camera_position;
			uint32_t flags;
			float radius;
			uint32_t seed;
		};

		std::shared_ptr<Sampler> _sampler = {};

		std::shared_ptr<ImageView> _positions = nullptr;
		std::shared_ptr<ImageView> _normals = nullptr;

		VkFormat _format;
		std::string _format_glsl = {};

		std::shared_ptr<ImageView> _target = nullptr;

		void createInternalResources();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::shared_ptr<ImageView> positions = nullptr;
			std::shared_ptr<ImageView> normals = nullptr;
			bool can_rt = false;
			uint32_t default_method = 1;
		};
		using CI = CreateInfo;

		AmbientOcclusion(CreateInfo const& ci);

		
		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & recorder, const Camera & camera);

		void declareGui(GuiContext & ctx);

		bool setCanRT(bool can_rt);

		const std::shared_ptr<ImageView>& target()const
		{
			return _target;
		}

		bool enable()const
		{
			return _enable;
		}
	};
}