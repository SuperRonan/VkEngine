#pragma once

#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/GraphicsProgram.hpp>
#include "RenderPass.hpp"

#include <vkl/VkObjects/Blending.hpp>

namespace vkl
{
	class GraphicsPipelineInstance : public PipelineInstance
	{
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::optional<VertexInputDescription> vertex_input;
			std::optional<VkPipelineInputAssemblyStateCreateInfo> input_assembly;
			MyVector<VkViewport> viewports;
			MyVector<VkRect2D> scissors;
			VkPipelineRasterizationStateCreateInfo rasterization;
			std::optional<VkPipelineRasterizationLineStateCreateInfoEXT> line_raster;
			VkPipelineMultisampleStateCreateInfo multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			MyVector<AttachmentBlending> attachments_blends;
			PipelineBlending common_blending = {};
			MyVector<VkDynamicState> dynamic;
			std::shared_ptr<RenderPassInstance> render_pass;
			uint32_t subpass_index = 0;
			std::shared_ptr<GraphicsProgramInstance> program;
		};
		using CI = CreateInfo;

	protected:

		friend class GraphicsPipeline;

		std::optional<VertexInputDescription> _vertex_input;
		std::optional<VkPipelineInputAssemblyStateCreateInfo> _input_assembly;
		MyVector<VkViewport> _viewports;
		MyVector<VkRect2D> _scissors;
		VkPipelineRasterizationStateCreateInfo _rasterization;
		std::optional<VkPipelineRasterizationLineStateCreateInfoEXT> _line_raster;
		VkPipelineMultisampleStateCreateInfo _multisampling;
		std::optional<VkPipelineDepthStencilStateCreateInfo> _depth_stencil = {};
		MyVector<AttachmentBlending> _attachments_blends;
		PipelineBlending _common_blending = {};
		MyVector<VkDynamicState> _dynamic;
		std::shared_ptr<RenderPassInstance> _render_pass;
		uint32_t _subpass_index = 0;

		void create();

	public:

		GraphicsPipelineInstance(CreateInfo const& ci);

		virtual ~GraphicsPipelineInstance() override = default;

		GraphicsProgramInstance* program()const
		{
			return static_cast<GraphicsProgramInstance*>(_program.get());
		}
	};

	class GraphicsPipeline : public Pipeline
	{
	public:

		constexpr static VkPipelineVertexInputStateCreateInfo VertexInputWithoutVertices()
		{
			VkPipelineVertexInputStateCreateInfo vertex_input{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = 0,
				.vertexAttributeDescriptionCount = 0,
			};
			return vertex_input;
		}

		constexpr static VkPipelineInputAssemblyStateCreateInfo InputAssemblyDefault(VkPrimitiveTopology topology)
		{
			VkPipelineInputAssemblyStateCreateInfo input_assembly{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = topology,
				.primitiveRestartEnable = VK_FALSE,
			};
			return input_assembly;
		}

		constexpr static VkViewport Viewport(VkRect2D const& rect)
		{
			VkViewport viewport{
				.x = static_cast<float>(rect.offset.x),
				.y = static_cast<float>(rect.offset.y),
				.width = static_cast<float>(rect.extent.width),
				.height = static_cast<float>(rect.extent.height),
				.minDepth = 0.0f, 
				.maxDepth = 1.0f,
			};
			return viewport;
		}

		constexpr static VkRect2D Scissor(VkExtent2D const& extent)
		{
			VkRect2D scissor{
				.offset = {0, 0},
				.extent = extent,
			};
			return scissor;
		}

		struct RasterizationState
		{
			VkBool32				depthClampEnable = VK_FALSE;
			VkBool32				rasterizerDiscardEnable = VK_FALSE;
			Dyn<VkPolygonMode>		polygonMode = {};
			Dyn<VkCullModeFlags>	cullMode = {};
			Dyn<VkFrontFace>		frontFace = {};
			VkBool32				depthBiasEnable = VK_FALSE;
			float					depthBiasConstantFactor = 0.0f;
			float					depthBiasClamp = 0.0f;
			float					depthBiasSlopeFactor = 0.0f;
			Dyn<float>				lineWidth = {};

			VkPipelineRasterizationStateCreateInfo value()const
			{
				VkPipelineRasterizationStateCreateInfo res{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.depthClampEnable = depthClampEnable,
					.rasterizerDiscardEnable = rasterizerDiscardEnable,
					.polygonMode = polygonMode.valueOr(VK_POLYGON_MODE_FILL),
					.cullMode = cullMode.valueOr(0),
					.frontFace = frontFace.valueOr(VK_FRONT_FACE_COUNTER_CLOCKWISE),
					.depthBiasEnable = depthBiasEnable,
					.depthBiasConstantFactor = depthBiasConstantFactor,
					.depthBiasClamp = depthBiasClamp,
					.depthBiasSlopeFactor = depthBiasSlopeFactor,
					.lineWidth = lineWidth.valueOr(1.0f),
				};
				return res;
			}
		};

		constexpr static VkPipelineRasterizationStateCreateInfo RasterizationDefault(VkCullModeFlags cull = VK_CULL_MODE_NONE, VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL, float line_width = 1.0f)
		{
			VkPipelineRasterizationStateCreateInfo rasterization{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = polygon_mode,
				.cullMode = cull,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE,
				.lineWidth = line_width,
			};
			return rasterization;
		}

		struct LineRasterizationState
		{
			Dyn<VkLineRasterizationModeKHR>	lineRasterizationMode;
			Dyn<uint32_t>	lineStippleFactor;
			Dyn<uint16_t>	lineStipplePattern;

			VkPipelineRasterizationLineStateCreateInfoEXT value() const
			{
				VkPipelineRasterizationLineStateCreateInfoEXT res{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT,
					.pNext = nullptr,
					.lineRasterizationMode = lineRasterizationMode.valueOr(VK_LINE_RASTERIZATION_MODE_DEFAULT_KHR),
					.stippledLineEnable = VK_FALSE,
					.lineStippleFactor = lineStippleFactor.valueOr(1),
					.lineStipplePattern = lineStipplePattern.valueOr(1),
				};
				if (res.lineStippleFactor != 1 || res.lineStipplePattern != 1)
				{
					res.stippledLineEnable = VK_TRUE;
				}
				return res;
			}
		};

		constexpr static VkPipelineRasterizationLineStateCreateInfoEXT LineRasterization(VkLineRasterizationModeEXT mode, uint32_t factor = 1, uint16_t pattern = 1)
		{
			VkPipelineRasterizationLineStateCreateInfoEXT res{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT,
				.pNext = nullptr,
				.lineRasterizationMode = mode,
				.stippledLineEnable = (factor != 1 && pattern != 1) ? VK_TRUE : VK_FALSE,
				.lineStippleFactor = factor,
				.lineStipplePattern = pattern,
			};
			return res;
		}

		struct MultisamplingState
		{
			enum class Flags : uint32_t
			{
				None = 0x0,
				AlphaToCoverageEnable = 0x1,
				AlphaToOneEnable = 0x2,
			};
			Dyn<VkSampleCountFlagBits> rasterization_samples = {};
			Dyn<float> min_sample_shading = {};
			Dyn<Flags> flags = {};

			VkPipelineMultisampleStateCreateInfo link();
		};

		constexpr static VkPipelineDepthStencilStateCreateInfo DepthStencilCloser(bool write_depth = true)
		{
			VkPipelineDepthStencilStateCreateInfo depth_stencil = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.depthTestEnable = VK_TRUE,
				.depthWriteEnable = write_depth ? VK_TRUE : VK_FALSE,
				.depthCompareOp = VK_COMPARE_OP_LESS,
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = VK_FALSE,
				.minDepthBounds = 0.0,
				.maxDepthBounds = 1.0,
			};
			return depth_stencil;
		}

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};

			std::optional<VertexInputDescription> vertex_input;
			std::optional<VkPipelineInputAssemblyStateCreateInfo> input_assembly;
			MyVector<VkViewport> viewports;
			MyVector<VkRect2D> scissors;

			RasterizationState rasterization;
			std::optional<LineRasterizationState> line_raster;
			MultisamplingState multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			MyVector<Dyn<AttachmentBlending>> attachments_blends;
			Dyn<PipelineBlending> common_blending = {};

			MyVector<VkDynamicState> dynamic;

			std::shared_ptr<RenderPass> render_pass;
			uint32_t subpass_index = 0;

			std::shared_ptr<GraphicsProgram> program;

			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

	protected:

		std::optional<VertexInputDescription> _vertex_input;
		std::optional<VkPipelineInputAssemblyStateCreateInfo> _input_assembly;
		MyVector<VkViewport> _viewports;
		MyVector<VkRect2D> _scissors;

		RasterizationState _rasterization;
		std::optional<LineRasterizationState> _line_raster;
		MultisamplingState _multisampling;
		std::optional<VkPipelineDepthStencilStateCreateInfo> _depth_stencil = {};
		MyVector<Dyn<AttachmentBlending>> _attachments_blends;
		Dyn<PipelineBlending> _common_blending;

		MyVector<VkDynamicState> _dynamic;

		std::shared_ptr<RenderPass> _render_pass;
		uint32_t _subpass_index = 0;

		virtual void createInstanceIFP() override;

		virtual bool checkInstanceParamsReturnInvalid() override;

	public:

		GraphicsPipeline(CreateInfo const& ci);

		virtual ~GraphicsPipeline() override;

		GraphicsProgram* program()const
		{
			return static_cast<GraphicsProgram*>(_program.get());
		}
	};

THAT_DECLARE_ENUM_CLASS_OPERATORS(GraphicsPipeline::MultisamplingState::Flags)

}
