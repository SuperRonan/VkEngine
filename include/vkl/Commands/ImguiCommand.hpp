#pragma once

#include <vkl/Execution/ExecutionContext.hpp>

#include <vkl/Commands/DeviceCommand.hpp>

#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/DescriptorPool.hpp>
#include <vkl/VkObjects/Queue.hpp>
#include <vkl/VkObjects/Fence.hpp>
#include <vkl/VkObjects/VkWindow.hpp>

#include <vkl/GUI/Context.hpp>

namespace vkl
{
	extern std::shared_ptr<DescriptorSetLayoutInstance> MakeImGuiDescriptorSetLayout(VkApplication* app, uint set);

	class ImGuiCommand : public DeviceCommand
	{
	protected:

		std::shared_ptr<Queue> _queue = nullptr;

		std::shared_ptr<VkWindow> _target_window = nullptr;
		std::vector<std::shared_ptr<Framebuffer>> _framebuffers = {};

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<DescriptorPool> _desc_pool = nullptr;

		std::shared_ptr<Shader> _custom_frag_shader = nullptr;
		VkSpecializationMapEntry _spec_entry;
		VkSpecializationInfo _specialization;
		VkSpecializationInfo _viewports_specialization;

		VkFormat _imgui_init_format = VK_FORMAT_B8G8R8A8_UNORM;
		VkSurfaceFormatKHR _viewports_format = {};

		Dyn<size_t> _index;

		MyVector<std::shared_ptr<Fence>> _fences_to_wait = {};

		ColorCorrectionInfo _color_correction_info = {};
		ColorCorrectionInfo _viewports_color_correction_info = {};

		std::shared_ptr<DescriptorSetLayoutInstance> _texture_set_layout;
		std::shared_ptr<DescriptorSetLayoutInstance> _sampler_set_layout;

		// Multi viewport support
		struct ViewportsFrameData
		{
			// Keep data for all viewports in the same struct
			MyVector<std::shared_ptr<VkObject>> objects;
		};
		// Per viewport frame in flight
		MyVector<ViewportsFrameData> _viewports_frame_data;
		MyVector<GUI::Context::ObjectUsedByCommand> _next_viewports_objects;
		uint32_t _viewports_resources_index = 0;

		bool _re_create_imgui_pipeline = true;

		void createRenderPassIFP();

		void createFramebuffers();

		void initImGui();

		void shutdownImGui();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<VkWindow> target_window = nullptr;
			std::shared_ptr<Queue> queue = nullptr;
		};
		using CI = CreateInfo;

		ImGuiCommand(CreateInfo const& ci);

		virtual ~ImGuiCommand() override;

		virtual void init()override;

		struct ExecutionInfo
		{
			size_t index;
			const std::set<std::shared_ptr<ImageViewInstance>> * images;
		};
		using EI = ExecutionInfo;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, ExecutionInfo const& ei);

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx) override;

		Executable with(ExecutionInfo const& ei);

		Executable operator()(ExecutionInfo const& ei)
		{
			return with(ei);
		}

		void setViewportsObjectsToKeepAliveMove(std::span<GUI::Context::ObjectUsedByCommand> objs)
		{
			// Check that objs are moved, not copied
			_next_viewports_objects.insert(_next_viewports_objects.end(), objs.begin(), objs.end());
		}

		void setViewportsObjectsToKeepAlive(const std::span<const GUI::Context::ObjectUsedByCommand> objs)
		{
			// Check that objs are moved, not copied
			_next_viewports_objects.insert(_next_viewports_objects.end(), objs.begin(), objs.end());
		}

		void renderViewports();

		virtual bool updateResources(UpdateContext & ctx) override;

		void setFenceToWait(uint index, std::shared_ptr<Fence> const& fence)
		{
			_fences_to_wait[index] = fence;
		}

		void clearFencesToWait()
		{
			for (uint32_t i = 0; i < _fences_to_wait.size32(); ++i)
			{
				_fences_to_wait[i].reset();
			}
		}

		std::shared_ptr<DescriptorSetLayoutInstance> const& textureSetLayout() const
		{
			return _texture_set_layout;
		}

		std::shared_ptr<DescriptorSetLayoutInstance> const& samplerSetLayout() const
		{
			return _sampler_set_layout;
		}
	};
}