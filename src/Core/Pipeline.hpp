#pragma once

#include "VkApplication.hpp"
#include "Program.hpp"
#include "RenderPass.hpp"

namespace vkl
{
	class PipelineInstance : public VkObject
	{
	protected:

		VkPipeline _handle = VK_NULL_HANDLE;
		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<ProgramInstance> _program;
		std::shared_ptr<RenderPass> _render_pass = nullptr;

	public:

		struct GraphicsCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkPipelineVertexInputStateCreateInfo vertex_input;
			VkPipelineInputAssemblyStateCreateInfo input_assembly;
			std::vector<VkViewport> viewports;
			std::vector<VkRect2D> scissors;
			VkPipelineRasterizationStateCreateInfo rasterization;
			VkPipelineMultisampleStateCreateInfo multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			std::vector<VkPipelineColorBlendAttachmentState> attachements_blends;
			std::shared_ptr<RenderPass> render_pass;
			std::shared_ptr<GraphicsProgramInstance> program;

		protected:
			mutable VkPipelineViewportStateCreateInfo _viewport;
			mutable VkPipelineColorBlendStateCreateInfo _blending;
			mutable std::vector<VkPipelineShaderStageCreateInfo> _shaders;
			mutable VkGraphicsPipelineCreateInfo _pipeline_ci;
		public:

			const VkGraphicsPipelineCreateInfo & assemble() const
			{
				{
					_viewport = {
						.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
						.viewportCount = (uint32_t)viewports.size(),
						.pViewports = viewports.data(),
						.scissorCount = (uint32_t)scissors.size(),
						.pScissors = scissors.data(),
					};

					_blending = {
						.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
						.logicOpEnable = VK_FALSE, // TODO
						.attachmentCount = (uint32_t)attachements_blends.size(),
						.pAttachments = attachements_blends.data(),
						.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
					};

					_shaders.resize(program->shaders().size());
					for (size_t i = 0; i < _shaders.size(); ++i)
					{
						_shaders[i] = program->shaders()[i]->getPipelineShaderStageCreateInfo();
					}
				}

				_pipeline_ci = VkGraphicsPipelineCreateInfo{
					.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stageCount = (uint32_t)_shaders.size(),
					.pStages = _shaders.data(),
					.pVertexInputState = &vertex_input,
					.pInputAssemblyState = &input_assembly,
					.pViewportState = &_viewport,
					.pRasterizationState = &rasterization,
					.pMultisampleState = &multisampling,
					.pDepthStencilState = depth_stencil.has_value() ? &depth_stencil.value() : nullptr,
					.pColorBlendState = &_blending,
					.pDynamicState = nullptr,
					.layout = *program->pipelineLayout(),
					.renderPass = *render_pass,
					.subpass = 0,
					.basePipelineHandle = VK_NULL_HANDLE,
					.basePipelineIndex = 0,
				};
				return _pipeline_ci;
			}
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

		constexpr static VkPipelineInputAssemblyStateCreateInfo InputAssemblyTriangleDefault()
		{
			VkPipelineInputAssemblyStateCreateInfo input_assembly{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				.primitiveRestartEnable = VK_FALSE,
			};
			return input_assembly;
		}

		constexpr static VkPipelineInputAssemblyStateCreateInfo InputAssemblyPointDefault()
		{
			VkPipelineInputAssemblyStateCreateInfo input_assembly{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

		constexpr static VkPipelineRasterizationStateCreateInfo RasterizationDefault()
		{
			VkPipelineRasterizationStateCreateInfo rasterization{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE,
				.lineWidth = 1.0f,
			};
			return rasterization;
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
				.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
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

			VkPipelineVertexInputStateCreateInfo vertex_input;
			VkPipelineInputAssemblyStateCreateInfo input_assembly;
			std::vector<VkViewport> viewports;
			std::vector<VkRect2D> scissors;

			VkPipelineRasterizationStateCreateInfo rasterization;
			VkPipelineMultisampleStateCreateInfo multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil = {};
			std::vector<VkPipelineColorBlendAttachmentState> attachements_blends;

			std::shared_ptr<RenderPass> render_pass;

			std::shared_ptr<GraphicsProgram> program;

		};
		
		struct ComputeCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::shared_ptr<ComputeProgram> program = nullptr;
		};

	protected:

		using ParentType = InstanceHolder<PipelineInstance>;

		
		GraphicsCreateInfo _gci;
		ComputeCreateInfo _cci;
		

		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<RenderPass> _render_pass = nullptr;
		std::shared_ptr<Program> _program = nullptr;

		void createInstance();

		void destroyInstance();

	public:

		Pipeline(GraphicsCreateInfo const& ci);

		Pipeline(ComputeCreateInfo const& ci);

		virtual ~Pipeline() override;

		constexpr const auto& program()const
		{
			return _program;
		}

		bool updateResources();
	};
}