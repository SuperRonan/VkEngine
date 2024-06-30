#pragma once

#include "ShaderCommand.hpp"
#include <Core/Rendering/Drawable.hpp>
#include <variant>
#include <Core/VkObjects/GraphicsPipeline.hpp>

#include <thatlib/src/utils/ExtensibleStorage.hpp>

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

		MyVector<VkClearValue> _clear_values;
		std::optional<VkClearColorValue> _clear_color;
		std::optional<VkClearDepthStencilValue> _clear_depth_stencil;

		VkViewport _viewport;

		virtual void clear() override;

		virtual void execute(ExecutionContext & ctx) override;

		virtual void recordDrawCalls(ExecutionContext & ctx) = 0;
	};

	struct AttachmentInfo
	{
		Dyn<VkFormat> format;
		Dyn<VkSampleCountFlagBits> samples;
	};
	struct ExternFramebufferInfo
	{
		MyVector<AttachmentInfo> color_attachments;
		std::optional<AttachmentInfo> detph_stencil_attchement;
		uint32_t layers = 1;
		bool multiview = false;
	};
	
	class GraphicsCommand : public ShaderCommand
	{
	public:
		
	protected:

		VkPrimitiveTopology _topology;
		VkPolygonMode _polygon_mode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags _cull_mode;
		VertexInputDescription _vertex_input_desc;
		std::shared_ptr<GraphicsProgram> _program;
		std::vector<std::shared_ptr<ImageView>> _attachements = {};
		std::shared_ptr<ImageView> _depth_stencil = nullptr;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::variant<std::shared_ptr<Framebuffer>, ExternFramebufferInfo> _framebuffer;

		std::optional<bool> _write_depth = {};
		std::optional<VkCompareOp> _depth_compare_op = VK_COMPARE_OP_LESS;
		std::optional<VkStencilOpState> _stencil_front_op = {};
		std::optional<VkStencilOpState> _stencil_back_op = {};

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
			VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			VertexInputDescription vertex_input_description = {};
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::optional<ExternFramebufferInfo> extern_framebuffer = {};
			std::vector<std::shared_ptr<ImageView>> targets = {};
			std::shared_ptr<ImageView> depth_stencil = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkCompareOp> depth_compare_op = {};
			std::optional<VkStencilOpState> stencil_front_op = {};
			std::optional<VkStencilOpState> stencil_back_op = {};
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

		std::shared_ptr<RenderPass> const& renderPass()const
		{
			return _render_pass;
		}
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

		
		that::ExS<BufferAndRangeInstance> _vertex_buffers;

		struct DrawCallInfo
		{
			Index name_begin = 0;
			Index pc_begin = 0;
			
			uint32_t name_size = 0;
			uint32_t pc_size = 0;

			uint32_t pc_offset = 0;
			uint32_t draw_count = 0;
			
			uint32_t instance_count = 0;
			uint32_t indirect_draw_stride = 5 * 4;
			
			BufferAndRangeInstance indirect_draw_buffer;
			
			BufferAndRangeInstance index_buffer = {};
			
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			uint32_t num_vertex_buffers = 0;
			
			Index vertex_buffer_begin = 0; // index in _vertex_buffers

			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
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

		friend struct VertexCommandTemplateProcessor;

		struct ShaderPaths
		{
			std::filesystem::path vertex_path;
			std::filesystem::path tess_control_path;
			std::filesystem::path tess_eval_path;
			std::filesystem::path geometry_path;
			std::filesystem::path fragment_path;
			DynamicValue<DefinitionsList> definitions;
		};

		ShaderPaths _shaders;

		DynamicValue<uint32_t> _draw_count = 0;

		virtual void createProgram() override;

		struct MyDrawCallInfo
		{
			using Index = ShaderCommandList::Index;
			Index name_begin = 0;
			Index pc_begin = 0;

			uint32_t name_size = 0;
			uint32_t pc_size = 0;

			uint32_t pc_offset = 0;
			uint32_t draw_count = 0;

			uint32_t instance_count = 0;
			uint32_t indirect_draw_stride = 5 * 4;

			BufferAndRange indirect_draw_buffer = {};

			BufferAndRange index_buffer = {};

			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			uint32_t num_vertex_buffers = 0;

			Index vertex_buffer_begin = 0;

			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VertexInputDescription vertex_input_desc = {};
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			DynamicValue<uint32_t> draw_count = {};
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::optional<ExternFramebufferInfo> extern_framebuffer = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_stencil = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkCompareOp> depth_compare_op = {};
			std::optional<VkStencilOpState> stencil_front_op = {};
			std::optional<VkStencilOpState> stencil_back_op = {};
			std::filesystem::path vertex_shader_path = {};
			std::filesystem::path tess_control_shader_path = {};
			std::filesystem::path tess_eval_shader_path = {};
			std::filesystem::path geometry_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<DefinitionsList> definitions = DefinitionsList();
			
			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};

			DrawType draw_type = {};
		};
		using CI = CreateInfo;

		template <bool CONST_VB>
		struct DrawCallInfoT
		{
			std::string_view name = {};
			const void* pc_data = nullptr;
			uint32_t pc_size = 0;
			
			uint32_t pc_offset = 0;
			uint32_t draw_count = 0;

			uint32_t instance_count = 0;
			uint32_t indirect_draw_stride = 5 * 4;

			BufferAndRange indirect_draw_buffer = {};

			BufferAndRange index_buffer = {};
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			uint32_t num_vertex_buffers = 0;

			typename std::conditional<CONST_VB, const BufferAndRange, BufferAndRange>::type * vertex_buffers = nullptr;

			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};
		using DrawCallInfo = DrawCallInfoT<false>;
		using DrawCallInfoConst = DrawCallInfoT<true>;
		
		struct DrawInfo : public ShaderCommandList
		{
			DrawType draw_type = DrawType::MAX_ENUM;	
			std::optional<VkViewport> viewport = {};

			MyVector<MyDrawCallInfo> calls;

			that::ExS<BufferAndRange> _vertex_buffers;

			std::shared_ptr<Framebuffer> extern_framebuffer = nullptr; 

			void pushBack(DrawCallInfo const& dci);
			void pushBack(DrawCallInfo && dci);
			void pushBack(DrawCallInfoConst const& dci);
			void pushBack(DrawCallInfoConst && dci);

			void clear();
		};
		using DI = DrawInfo;
		

		VertexCommand(CreateInfo const& ci);

		virtual bool updateResources(UpdateContext & ctx) override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DrawInfo const& di);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext & ctx, DrawInfo && di);

		Executable with(DrawInfo const& di);
		Executable with(DrawInfo && di);
		
		Executable operator()(DrawInfo const& di)
		{
			return with(di);
		}

		Executable operator()(DrawInfo && di)
		{
			return with(std::move(di));
		}

		struct SingleDrawInfo
		{
			uint32_t draw_count = 0;
			uint32_t instance_count = 1;
			const void * pc_data = nullptr;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			std::optional<VkViewport> viewport = {};
		};
		
		Executable with(SingleDrawInfo const& sdi);
		Executable with(SingleDrawInfo && sdi);

		Executable operator()(SingleDrawInfo const& sdi)
		{
			return with(sdi);
		}
		
		Executable operator()(SingleDrawInfo && sdi)
		{
			return with(std::move(sdi));
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
			Index name_begin = 0;
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = makeUniformExtent3D(0);
			std::shared_ptr<DescriptorSetAndPoolInstance> set = nullptr;
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

		friend struct MeshCommandTemplateProcessor;
		
		struct ShaderPaths
		{
			std::filesystem::path task_path;
			std::filesystem::path mesh_path;
			std::filesystem::path fragment_path;
			DynamicValue<DefinitionsList> definitions;
		};

		ShaderPaths _shaders;

		DynamicValue<VkExtent3D> _extent = {};
		bool _dispatch_threads = false;

		virtual void createProgram() override;

		struct MyDrawCallInfo
		{
			using Index = ShaderCommandList::Index;
			Index name_begin = 0;
			Index pc_begin = 0;
			uint32_t name_size = 0;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = makeUniformExtent3D(0);
			std::shared_ptr<DescriptorSetAndPool> set;
		};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
			DynamicValue<VkExtent3D> extent = {};
			bool dispatch_threads = false;
			std::optional<VkLineRasterizationModeEXT> line_raster_mode = {};
			MultiDescriptorSetsLayouts sets_layouts = {};
			std::vector<ShaderBindingDescription> bindings = {};
			std::optional<ExternFramebufferInfo> extern_framebuffer = {};
			std::vector<std::shared_ptr<ImageView>> color_attachements = {};
			std::shared_ptr<ImageView> depth_stencil = nullptr;
			std::optional<bool> write_depth = {};
			std::optional<VkCompareOp> depth_compare_op = {};
			std::optional<VkStencilOpState> stencil_front_op = {};
			std::optional<VkStencilOpState> stencil_back_op = {};
			std::filesystem::path task_shader_path = {};
			std::filesystem::path mesh_shader_path = {};
			std::filesystem::path fragment_shader_path = {};
			DynamicValue<DefinitionsList> definitions = DefinitionsList();

			std::optional<VkClearColorValue> clear_color = {};
			std::optional<VkClearDepthStencilValue> clear_depth_stencil = {};

			std::optional<VkPipelineColorBlendAttachmentState> blending = {};
		};
		using CI = CreateInfo;

		MeshCommand(CreateInfo const& ci);

		struct DrawCallInfo
		{
			std::string_view name = {};
			const void * pc_data = nullptr;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			VkExtent3D extent = makeUniformExtent3D(0);
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};

		struct DrawInfo : public ShaderCommandList
		{
			DrawType draw_type = DrawType::MAX_ENUM;
			bool dispatch_threads = true;

			MyVector<MyDrawCallInfo> draw_list;
			std::shared_ptr<Framebuffer> extern_framebuffer = nullptr;

			void pushBack(DrawCallInfo const& dci);
			void pushBack(DrawCallInfo && dci);

			void clear();
		};
		using DI = DrawInfo;

		virtual bool updateResources(UpdateContext& ctx) override;

		virtual std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx) override;

		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, DrawInfo const& di);
		std::shared_ptr<ExecutionNode> getExecutionNode(RecordContext& ctx, DrawInfo && di);

		Executable with(DrawInfo const& di);
		Executable with(DrawInfo && di);

		Executable operator()(DrawInfo const& di)
		{
			return with(di);
		}

		Executable operator()(DrawInfo && di)
		{
			return with(std::move(di));
		}

		struct SingleDrawInfo
		{
			std::optional<VkExtent3D> extent;
			std::optional<bool> dispatch_threads = false;
			const void * pc_data = nullptr;
			uint32_t pc_size = 0;
			uint32_t pc_offset = 0;
			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		};
		Executable with(SingleDrawInfo const& sdi);
		Executable with(SingleDrawInfo && sdi);

		Executable operator()(SingleDrawInfo const& sdi)
		{
			return with(sdi);
		}

		Executable operator()(SingleDrawInfo && sdi)
		{
			return with(std::move(sdi));
		}

		VkExtent3D getWorkgroupsDispatchSize(VkExtent3D threads)const
		{
			const std::shared_ptr<GraphicsProgramInstance>& prog = std::reinterpret_pointer_cast<GraphicsProgramInstance>(_program->instance());
			const VkExtent3D lcl = prog->localSize(); 
			return VkExtent3D{
				.width = std::divCeil(threads.width, lcl.width),
				.height = std::divCeil(threads.height, lcl.height),
				.depth = std::divCeil(threads.depth, lcl.depth),
			};
		}
	};

	//class FragCommand : public GraphicsCommand
	//{
	//protected:

	//	virtual void createProgram() override;

	//public:

	//	struct CreateInfo
	//	{
	//		VkApplication* app = nullptr;
	//		std::string name = {};
	//		std::vector<ShaderBindingDescription> bindings = {};
	//		std::vector<std::shared_ptr<ImageView>> color_attachements = {};
	//		std::shared_ptr<ImageView> depth_buffer = nullptr;
	//		std::filesystem::path fragment_shader_path = {};
	//		DynamicValue<DefinitionsList> definitions;
	//	};

	//	using CI = CreateInfo;

	//	FragCommand(CreateInfo const& ci);

	//	virtual void init() override;

	//	//virtual void recordDraw(CommandBuffer& cmd, ExecutionContext& context, void * user_data) override;

	//};
}