#pragma once

#include <vkl/Execution/Module.hpp>

#include <vkl/Commands/GraphicsCommand.hpp>
#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/Rendering/Camera.hpp>
#include <vkl/Rendering/RenderWorldBasis.hpp>

namespace vkl
{
	class BSDFViewer : public Module
	{
	public:

	protected:

		MultiDescriptorSetsLayouts _sets_layouts;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		std::shared_ptr<ImageView> _target = nullptr;

		Camera * _camera = nullptr;

		// In radians
		bool _hemisphere = true;
		float _inclination = glm::radians(45.0f);

		float _common_alpha = 0.9;

		ImVec4 _clear_color = ImVec4(0, 0, 0, 0);

		uint32_t _alignment = 0;
		uint32_t _resolution = 0;
		Dyn<uint32_t> _num_functions;
		Dyn<VkExtent2D> _sphere_resolution;

		MyVector<Vector4f> _colors;

		VkFormat _format = VK_FORMAT_R32_SFLOAT;
		std::shared_ptr<ImageView> _functions_image = nullptr;
		
		std::shared_ptr<HostManagedBuffer> _ubo;

		VkPolygonMode _polygon_mode_3D = VK_POLYGON_MODE_FILL;

		AttachmentBlending _blending;

		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;

		std::shared_ptr<MeshCommand> _render_3D_mesh;
		std::shared_ptr<MeshCommand> _render_2D_mesh;
	
		std::shared_ptr<VertexCommand> _render_in_vector;
	
		std::shared_ptr<RenderWorldBasis> _render_world_basis = nullptr;

		void createInternals();

	public:
		
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<ImageView> target = nullptr;
			Camera * camera = nullptr;
			MultiDescriptorSetsLayouts sets_layouts;
		};
		using CI = CreateInfo;

		BSDFViewer(CreateInfo const& ci);

		virtual ~BSDFViewer() override;

		void updateResources(UpdateContext & ctx);

		void execute(ExecutionRecorder & exec);

		void declareGUI(GuiContext & ctx);

		std::shared_ptr<ImageView> target() const
		{
			return _target;
		}
	};
}