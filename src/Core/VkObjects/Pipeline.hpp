#pragma once

#include <Core/App/VkApplication.hpp>
#include "Program.hpp"
#include "RenderPass.hpp"

namespace vkl
{
	class PipelineInstance : public AbstractInstance
	{
	protected:

		VkPipeline _handle = VK_NULL_HANDLE;
		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<ProgramInstance> _program;
		std::shared_ptr<PipelineLayoutInstance> _layout;
		std::shared_ptr<RenderPass> _render_pass = nullptr;

	public:

		struct GraphicsCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::optional<VertexInputDescription> vertex_input;
			std::optional<VkPipelineInputAssemblyStateCreateInfo> input_assembly;
			std::vector<VkViewport> viewports;
			std::vector<VkRect2D> scissors;
			mutable VkPipelineRasterizationStateCreateInfo rasterization;
			std::optional<VkPipelineRasterizationLineStateCreateInfoEXT> line_raster;
			VkPipelineMultisampleStateCreateInfo multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			std::vector<VkPipelineColorBlendAttachmentState> attachements_blends;
			std::shared_ptr<RenderPass> render_pass;
			std::shared_ptr<GraphicsProgramInstance> program;
			std::vector<VkDynamicState> dynamic;

		protected:
			mutable VkPipelineVertexInputStateCreateInfo _vk_vertex_input;
			mutable VkPipelineViewportStateCreateInfo _viewport;
			mutable VkPipelineColorBlendStateCreateInfo _blending;
			mutable std::vector<VkPipelineShaderStageCreateInfo> _shaders;
			mutable VkPipelineDynamicStateCreateInfo _dynamic_state;
			mutable VkGraphicsPipelineCreateInfo _pipeline_ci;
		public:

			const VkGraphicsPipelineCreateInfo & assemble() const;
		};

		struct ComputeCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ComputeProgramInstance> program = nullptr;
		};

		PipelineInstance(GraphicsCreateInfo const& gci);

		PipelineInstance(ComputeCreateInfo const& cci);

		virtual ~PipelineInstance() override;

		constexpr VkPipeline pipeline()const noexcept
		{
			return _handle;
		}

		constexpr auto handle()const noexcept
		{
			return pipeline();
		}

		constexpr VkPipelineBindPoint binding()const noexcept
		{
			return _binding;
		}

		constexpr operator VkPipeline()const
		{
			return _handle;
		}

		constexpr const auto& program()const
		{
			return _program;
		}

	};


	class Pipeline : public InstanceHolder<PipelineInstance>
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

		constexpr static VkViewport Viewport(VkExtent2D const& extent)
		{
			VkViewport viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = (float)extent.width,
				.height = (float)extent.height,
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

		constexpr static VkPipelineMultisampleStateCreateInfo MultisampleOneSample()
		{
			VkPipelineMultisampleStateCreateInfo multisampling{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.sampleShadingEnable = VK_FALSE,
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = VK_FALSE,
				.alphaToOneEnable = VK_FALSE,
			};
			return multisampling;
		}

		constexpr static VkPipelineMultisampleStateCreateInfo MultisampleState(VkSampleCountFlagBits samples, std::optional<float> sample_shading = {})
		{
			VkPipelineMultisampleStateCreateInfo multisampling{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = samples,
				.sampleShadingEnable = sample_shading.has_value() ? VK_TRUE : VK_FALSE,
				.minSampleShading = sample_shading.value_or(0),
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = VK_FALSE,
				.alphaToOneEnable = VK_FALSE,
			};
			return multisampling;
		}

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

		constexpr static VkPipelineColorBlendAttachmentState BlendAttachementNoBlending()
		{
			VkPipelineColorBlendAttachmentState color_blend_attachement{
				.blendEnable = VK_FALSE,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			};
			return color_blend_attachement;
		}

		constexpr static VkPipelineColorBlendAttachmentState BlendAttachementBlendingAlphaDefault()
		{
			VkPipelineColorBlendAttachmentState color_blend_attachement{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.alphaBlendOp = VK_BLEND_OP_MAX,
				.colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT |
					VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT,
			};
			return color_blend_attachement;
		}


		struct GraphicsCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};

			std::optional<VertexInputDescription> vertex_input;
			std::optional<VkPipelineInputAssemblyStateCreateInfo> input_assembly;
			std::vector<VkViewport> viewports;
			std::vector<VkRect2D> scissors;

			VkPipelineRasterizationStateCreateInfo rasterization;
			std::optional<VkPipelineRasterizationLineStateCreateInfoEXT> line_raster;
			VkPipelineMultisampleStateCreateInfo multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			std::vector<VkPipelineColorBlendAttachmentState> attachements_blends;

			std::shared_ptr<RenderPass> render_pass;

			std::shared_ptr<GraphicsProgram> program;

			std::vector<VkDynamicState> dynamic;

			Dyn<bool> hold_instance = true;
		};
		
		struct ComputeCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ComputeProgram> program = nullptr;
			Dyn<bool> hold_instance = true;
		};

	protected:

		using ParentType = InstanceHolder<PipelineInstance>;

		
		GraphicsCreateInfo _gci;
		ComputeCreateInfo _cci;
		

		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Program> _program = nullptr;

		mutable std::shared_ptr<AsynchTask> _create_instance_task = nullptr;


		void createInstanceIFP();

		virtual void destroyInstanceIFN() override;

	public:

		Pipeline(GraphicsCreateInfo const& ci);

		Pipeline(ComputeCreateInfo const& ci);

		virtual ~Pipeline() override;

		constexpr const auto& program()const
		{
			return _program;
		}

		bool updateResources(UpdateContext & ctx);

		bool hasInstanceOrIsPending() const
		{
			return _inst || _create_instance_task;
		}

		void waitForInstanceCreationIFN();

		std::shared_ptr<PipelineInstance> getInstanceWaitIFN();
	};
}