#pragma once

#include "ShaderCommand.hpp"
#include <Core/Rendering/Drawable.hpp>

namespace vkl
{
	class GraphicsCommandNode : public ShaderCommandNode
	{
	public:
		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		GraphicsCommandNode(CreateInfo const& ci);

		std::shared_ptr<RenderPassInstance> _render_pass;
		std::shared_ptr<FramebufferInstance> _framebuffer;
		std::shared_ptr<ImageViewInstance> _depth_stencil;

		std::optional<VkClearColorValue> _clear_color;
		std::optional<VkClearDepthStencilValue> _clear_depth_stencil;

		VkViewport _viewport;

		virtual void clear() override;

		virtual void execute(ExecutionContext & ctx) override;

		virtual void recordDrawCalls(ExecutionContext & ctx) = 0;
	};
	class GraphicsCommand : public ShaderCommand
	{
	public:
		
	protected:

		VkPrimitiveTopology _topology;
		VkCullModeFlags _cull_mode;
		VertexInputDescription _vertex_input_desc;
		std::shared_ptr<GraphicsProgram> _program;
		std::vector<std::shared_ptr<ImageView>> _attachements = {};
		std::shared_ptr<ImageView> _depth = nullptr;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Framebuffer> _framebuffer = nullptr;

		std::optional<bool> _write_depth = {};

		std::optional<VkClearColorValue> _clear_color = {};
		std::optional<VkClearDepthStencilValue> _clear_depth_stencil = {};

		std::optional<VkPipelineColorBlendAttachmentState> _blending = {};

		std::optional<VkLineRasterizationModeEXT> _line_raster_mode = {};

		DrawType _draw_type = {};

		virtual void createProgram() = 0;

		virtual void createGraphicsResources();

		void createPipeline();

		virtual void populateFramebufferResources(GraphicsCommandNode & node);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			VertexInputDescription vertex_input_description = {};
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> targets = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};
			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
			DrawType draw_type = {};
		};

		GraphicsCommand(CreateInfo const& ci);

		virtual void init() override;
		
		struct DrawInfo
		{
			VkViewport viewport;
		};

		virtual bool updateResources(UpdateContext & ctx) override;
	};






	class VertexCommandNode : public GraphicsCommandNode
	{
	public:
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		VertexCommandNode(CreateInfo const& ci);

		
		MyVector<BufferAndRangeInstance> _vertex_buffers;

		struct DrawCallInfo
		{
			std::string name = {};
			
			uint32_t draw_count = 0;
			uint32_t instance_count = 0;
			BufferAndRangeInstance index_buffer = {};
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			uint32_t vertex_buffer_begin = 0; // index in _vertex_buffers
			uint32_t num_vertex_buffers = 0;

			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
			PushConstant pc = {};
		};

		MyVector<DrawCallInfo> _draw_list;
		DrawType _draw_type = DrawType::MAX_ENUM;

		virtual void clear() override;

		MyVector<VkBuffer> _vb_bind;
		MyVector<VkDeviceSize> _vb_offsets;

		virtual void recordDrawCalls(ExecutionContext & ctx) override;
	};

	// Uses the "classic" vertex pipeline
	class VertexCommand : public GraphicsCommand
	{
	protected:

		struct ShaderPaths
		{
			std::filesystem::path vertex_path;
			std::filesystem::path tess_control_path;
			std::filesystem::path tess_eval_path;
			std::filesystem::path geometry_path;
			std::filesystem::path fragment_path;
			DynamicValue<std::vector<std::string>> definitions;
		};

		ShaderPaths _shaders;

		DynamicValue<uint32_t> _draw_count = 0;

		virtual void createProgram() override;

		struct DrawInfo;
		void populateDrawCallsResources(VertexCommandNode & node, DrawInfo const& di);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VertexInputDescription vertex_input_desc = {};
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			DynamicValue<uint32_t> draw_count = {};
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::filesystem::path vertex_shader_path = {};
			std::filesystem::path tess_control_shader_path = {};
			std::filesystem::path tess_eval_shader_path = {};
			std::filesystem::path geometry_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<std::vector<std::string>> definitions = std::vector<std::string>();
			
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};

			DrawType draw_type = {};
		};
		using CI = CreateInfo;

		
		struct DrawInfo
		{
			DrawType draw_type = DrawType::MAX_ENUM;	
			std::optional<VkViewport> viewport = {};

			VertexDrawList draw_list = {};

			void clear()
			{
				draw_type = DrawType::MAX_ENUM;
				viewport.reset();
				draw_list.clear();
			}
		};
		using DI = DrawInfo;
		

		VertexCommand(CreateInfo const& ci);

		virtual bool updateResources(UpdateContext & ctx) override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DrawInfo const& di);

		Executable with(DrawInfo const& di);
		
		Executable operator()(DrawInfo const& di)
		{
			return with(di);
		}

		struct SingleDrawInfo
		{
			uint32_t draw_count;
			PushConstant pc;
			std::optional<VkViewport> viewport = {};
		};
		Executable with(SingleDrawInfo const& sdi)
		{
			DrawInfo di{
				.draw_type = DrawType::Draw,
				.viewport = sdi.viewport,
			};
			di.draw_list.push_back(VertexDrawList::DrawCallInfo{
				.draw_count = sdi.draw_count,
				.instance_count = 1,
				.pc = sdi.pc,
			});
			return with(di);
		}
		Executable operator()(SingleDrawInfo const& sdi)
		{
			return with(sdi);
		}
	};










	class MeshCommandNode : public GraphicsCommandNode
	{
	public:
		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;
		MeshCommandNode(CreateInfo const& ci);


		struct DrawCallInfo
		{
			std::string name = {};
			VkExtent3D extent = makeZeroExtent3D();
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
			PushConstant pc = {};
		};

		MyVector<DrawCallInfo> _draw_list;
		DrawType _draw_type = DrawType::MAX_ENUM;

		virtual void clear() override;

		virtual void recordDrawCalls(ExecutionContext& ctx) override;
	};


	// Uses the mesh pipeline
	class MeshCommand : public GraphicsCommand
	{
	protected:
		
		struct ShaderPaths
		{
			std::filesystem::path task_path;
			std::filesystem::path mesh_path;
			std::filesystem::path fragment_path;
			DynamicValue<std::vector<std::string>> definitions;
		};

		ShaderPaths _shaders;

		DynamicValue<VkExtent3D> _extent = {};
		bool _dispatch_threads = false;

		virtual void createProgram() override;

		struct DrawInfo;
		void populateDrawCallsResources(MeshCommandNode & node, DrawInfo const& di);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			DynamicValue<VkExtent3D> extent = {};
			bool dispatch_threads = false;
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::optional<bool> write_depth = {};
			std::filesystem::path task_shader_path = {};
			std::filesystem::path mesh_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<std::vector<std::string>> definitions = std::vector<std::string>();

			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
		};
		using CI = CreateInfo;

		MeshCommand(CreateInfo const& ci);

		struct DrawCallInfo
		{
			std::string name;
			PushConstant pc = {};
			VkExtent3D extent = {};
			std::shared_ptr<DescriptorSetAndPool> set;
		};

		struct DrawInfo
		{
			DrawType draw_type;
			bool dispatch_threads = true;
			MyVector<DrawCallInfo> draw_list;
		};
		using DI = DrawInfo;

		virtual bool updateResources(UpdateContext& ctx) override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, DrawInfo const& di);

		Executable with(DrawInfo const& di);

		Executable operator()(DrawInfo const& di)
		{
			return with(di);
		}

		struct SingleDrawInfo
		{
			std::optional<VkExtent3D> extent;
			std::optional<bool> dispatch_threads = false;
			PushConstant pc = {};
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};
		Executable with(SingleDrawInfo const& sdi)
		{
			DrawInfo di{
				.draw_type = DrawType::Dispatch,
				.dispatch_threads = sdi.dispatch_threads.value_or(_dispatch_threads),
				.draw_list {
					DrawCallInfo{
						.pc = sdi.pc.empty() ? _pc : sdi.pc,
						.extent = sdi.extent.value_or(_extent.value()),
						.set = sdi.set,
					},
				},
			};
			return with(di);
		}

		Executable operator()(SingleDrawInfo const& sdi)
		{
			return with(sdi);
		}

		VkExtent3D getWorkgroupsDispatchSize(VkExtent3D threads)const
		{
			const std::shared_ptr<GraphicsProgramInstance>& prog = std::dynamic_pointer_cast<GraphicsProgramInstance>(_program->instance());
			const VkExtent3D lcl = prog->localSize(); 
			return VkExtent3D{
				.width = std::divCeil(threads.width, lcl.width),
				.height = std::divCeil(threads.height, lcl.height),
				.depth = std::divCeil(threads.depth, lcl.depth),
			};
		}
	};

	class FragCommand : public GraphicsCommand
	{
	protected:

		virtual void createProgram() override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_buffer = nullptr;
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<std::vector<std::string>> definitions;
		};

		using CI = CreateInfo;

		FragCommand(CreateInfo const& ci);

		virtual void init() override;

		//virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context, void * user_data) override;

	};
}