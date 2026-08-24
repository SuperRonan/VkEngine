#pragma once

#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/Executor.hpp>

#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/RayTracingCommand.hpp>

#include <vkl/Rendering/Camera.hpp>


namespace vkl
{
	namespace GUI
	{
		class AmbientOcclusionInspector;
	}
	class AmbientOcclusion : public Module
	{
	public:

		enum class Method
		{
			SSAO = 0,
			RTAO = 1,
			RQAO = 2,
		};
		
		struct UBO
		{
			float radius;
		};

	protected:

		// Controls if the Acceleration structure is available (set by the caller from the scene)
		bool _can_rt = false;
		bool _enable = true;
		int _ao_samples = 16;
		float _radius = 0.02;
		int _downscale = 2;
		uint32_t _seed = 0;

		Method _method = Method::SSAO;

		MultiDescriptorSetsLayouts _sets_layouts = {};

		std::string _method_glsl;

		std::shared_ptr<ComputeCommand> _ssao_compute_command = nullptr;
		std::shared_ptr<ComputeCommand> _rqao_compute_command = nullptr;
		std::shared_ptr<RayTracingCommand> _rtao_command = nullptr;

		struct CommandPC
		{
			Vector3f camera_position;
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
			Method default_method = Method::RTAO;
		};
		using CI = CreateInfo;

		AmbientOcclusion(CreateInfo const& ci);

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & recorder, const Camera & camera);

		using InspectorType = GUI::AmbientOcclusionInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(GUI::InspectorMakeInfo const& imi) override;

		bool setCanRT(bool can_rt);

		const std::shared_ptr<ImageView>& target()const
		{
			return _target;
		}

		bool enable()const
		{
			return _enable;
		}

		// 1 -> need RT
		// 2 -> need RQ
		uint32_t needRTOrRQ()const
		{
			uint32_t res = 0;
			if (enable())
			{
				if (_method == Method::RTAO)
				{
					res |= 1;
				}
				if (_method == Method::RQAO)
				{
					res |= 2;
				}
			}
			return res;
		}
	};
}