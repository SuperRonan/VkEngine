#pragma once

#include "VkApplication.hpp"
#include "Program.hpp"

namespace vkl
{
	class Pipeline : public VkObject
	{
	public:

		struct GraphicsCreateInfo
		{
			VkPipelineVertexInputStateCreateInfo _vertex_input;
			VkPipelineInputAssemblyStateCreateInfo _input_assembly;
			std::vector<VkViewport> _viewports;
			std::vector<VkRect2D> _scissors;
			VkPipelineViewportStateCreateInfo _viewport;
			VkPipelineRasterizationStateCreateInfo _rasterization;
			VkPipelineMultisampleStateCreateInfo _multisampling;
			std::optional<VkPipelineDepthStencilStateCreateInfo> _depth_stencil = {};
			std::vector<VkPipelineColorBlendAttachmentState> _attachements_blends;
			VkPipelineColorBlendStateCreateInfo _blending;
			VkGraphicsPipelineCreateInfo _pipeline_ci;
			VkRenderPass _render_pass;

			std::vector<VkPipelineShaderStageCreateInfo> _shaders;

			std::shared_ptr<GraphicsProgram> _program;

			std::string name = {};
			
			constexpr void setViewport(std::vector<VkViewport>&& viewports, std::vector<VkRect2D>&& scissors)
			{
				_viewports = std::move(viewports);
				_scissors = std::move(scissors);
				_viewport = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
					.viewportCount = (uint32_t)_viewports.size(),
					.pViewports = _viewports.data(),
					.scissorCount = (uint32_t)_scissors.size(),
					.pScissors = _scissors.data(),
				};
				
			}

			constexpr void setColorBlending(std::vector<VkPipelineColorBlendAttachmentState>&& blendings)
			{
				_attachements_blends = std::move(blendings);

				_blending = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
					.logicOpEnable = VK_FALSE, // TODO
					.attachmentCount = (uint32_t)_attachements_blends.size(),
					.pAttachments = _attachements_blends.data(),
					.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
				};
			}
			
			constexpr void assemble()
			{
				_shaders.resize(_program->shaders().size());
				for (size_t i = 0; i < _shaders.size(); ++i)	_shaders[i] = _program->shaders()[i]->getPipelineShaderStageCreateInfo();

				_pipeline_ci = VkGraphicsPipelineCreateInfo{
					.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stageCount = (uint32_t)_shaders.size(),
					.pStages = _shaders.data(),
					.pVertexInputState = &_vertex_input,
					.pInputAssemblyState = &_input_assembly,
					.pViewportState = &_viewport,
					.pRasterizationState = &_rasterization,
					.pMultisampleState = &_multisampling,
					.pDepthStencilState = _depth_stencil.has_value() ? &_depth_stencil.value() : nullptr,
					.pColorBlendState = &_blending,
					.pDynamicState = nullptr,
					.layout = _program->pipelineLayout(),
					.renderPass = _render_pass,
					.subpass = 0,
					.basePipelineHandle = VK_NULL_HANDLE,
					.basePipelineIndex = 0,
				};
			}
		};

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

		constexpr static VkPipelineDepthStencilStateCreateInfo DepthStencilCloser()
		{
			VkPipelineDepthStencilStateCreateInfo depth_stencil = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.depthTestEnable  = VK_TRUE,
				.depthWriteEnable = VK_TRUE,
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



	protected:

		VkPipeline _handle = VK_NULL_HANDLE;
		VkPipelineBindPoint _binding = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		std::shared_ptr<Program> _program = nullptr;

	public:

		constexpr Pipeline() noexcept = default;

		constexpr Pipeline(VkApplication* app, VkPipeline handle, VkPipelineBindPoint type) noexcept:
			VkObject(app),
			_handle(handle),
			_binding(type)
		{}

		Pipeline(Pipeline const&) = delete;

		constexpr Pipeline(Pipeline&& other) noexcept :
			VkObject(std::move(other)),
			_handle(other._handle),
			_binding(other._binding)
		{
			other._handle = VK_NULL_HANDLE;
		}

		Pipeline(GraphicsCreateInfo & ci);

		Pipeline(std::shared_ptr<ComputeProgram> compute_program, std::string const& name = {});

		Pipeline& operator=(Pipeline const&) = delete;

		Pipeline& operator=(Pipeline&& other) noexcept;

		virtual ~Pipeline();

		void createPipeline(VkGraphicsPipelineCreateInfo const& ci);

		void createPipeline(VkComputePipelineCreateInfo const& ci);

		void destroyPipeline();

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

		constexpr auto& program()
		{
			return _program;
		}

		constexpr const auto& program()const
		{
			return _program;
		}
	};
}