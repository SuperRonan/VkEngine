#pragma once

#include <vkl/VkObjects/Pipeline.hpp>
#include <vkl/VkObjects/GraphicsProgram.hpp>
#include "RenderPass.hpp"

#include <vkl/VkObjects/Blending.hpp>

namespace vkl
{
	enum class CompareOp : uint32_t
	{
		Never = VK_COMPARE_OP_NEVER,
		Less = VK_COMPARE_OP_LESS,
		Equal = VK_COMPARE_OP_EQUAL,
		LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
		Greater = VK_COMPARE_OP_GREATER,
		NotEqual = VK_COMPARE_OP_NOT_EQUAL,
		GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
		Always = VK_COMPARE_OP_ALWAYS,
		_BitCount = 3,
	};
	enum class StencilOp : uint32_t
	{
		Keep = VK_STENCIL_OP_KEEP,
		Zero = VK_STENCIL_OP_ZERO,
		Replace = VK_STENCIL_OP_REPLACE,
		IncrementClamp = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
		DecrementClamp = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
		Invert = VK_STENCIL_OP_INVERT,
		IncrementWrap = VK_STENCIL_OP_INCREMENT_AND_WRAP,
		DecrementWrap = VK_STENCIL_OP_DECREMENT_AND_WRAP,
		_BitCount = 3,
	};

	THAT_DECLARE_ENUM_CLASS_OPERATORS(CompareOp)
	THAT_DECLARE_ENUM_CLASS_OPERATORS(StencilOp)

	namespace impl
	{
		enum class StencilOpStateFlags : uint32_t
		{
			_FailOpBitOffset = 0,
			FailOpKeep = (StencilOp::Keep << _FailOpBitOffset),
			FailOpZero = (StencilOp::Zero << _FailOpBitOffset),
			FailOpReplace = (StencilOp::Replace << _FailOpBitOffset),
			FailOpIncrementClamp = (StencilOp::IncrementClamp << _FailOpBitOffset),
			FailOpDecrementClamp = (StencilOp::DecrementClamp << _FailOpBitOffset),
			FailOpInvert = (StencilOp::Invert << _FailOpBitOffset),
			FailOpIncrementWrap = (StencilOp::IncrementWrap << _FailOpBitOffset),
			FailOpDecrementWrap = (StencilOp::DecrementWrap << _FailOpBitOffset),

			_PassOpBitOffset = _FailOpBitOffset + static_cast<uint32_t>(StencilOp::_BitCount),
			PassOpKeep = (StencilOp::Keep << _PassOpBitOffset),
			PassOpZero = (StencilOp::Zero << _PassOpBitOffset),
			PassOpReplace = (StencilOp::Replace << _PassOpBitOffset),
			PassOpIncrementClamp = (StencilOp::IncrementClamp << _PassOpBitOffset),
			PassOpDecrementClamp = (StencilOp::DecrementClamp << _PassOpBitOffset),
			PassOpInvert = (StencilOp::Invert << _PassOpBitOffset),
			PassOpIncrementWrap = (StencilOp::IncrementWrap << _PassOpBitOffset),
			PassOpDecrementWrap = (StencilOp::DecrementWrap << _PassOpBitOffset),

			_DepthFailOpBitOffset = _PassOpBitOffset + static_cast<uint32_t>(StencilOp::_BitCount),
			DepthFailOpKeep = (StencilOp::Keep << _DepthFailOpBitOffset),
			DepthFailOpZero = (StencilOp::Zero << _DepthFailOpBitOffset),
			DepthFailOpReplace = (StencilOp::Replace << _DepthFailOpBitOffset),
			DepthFailOpIncrementClamp = (StencilOp::IncrementClamp << _DepthFailOpBitOffset),
			DepthFailOpDecrementClamp = (StencilOp::DecrementClamp << _DepthFailOpBitOffset),
			DepthFailOpInvert = (StencilOp::Invert << _DepthFailOpBitOffset),
			DepthFailOpIncrementWrap = (StencilOp::IncrementWrap << _DepthFailOpBitOffset),
			DepthFailOpDecrementWrap = (StencilOp::DecrementWrap << _DepthFailOpBitOffset),

			_CompareOpBitOffset = _DepthFailOpBitOffset + static_cast<uint32_t>(StencilOp::_BitCount),
			CompareOpNever = (CompareOp::Never << _CompareOpBitOffset),
			CompareOpLess = (CompareOp::Less << _CompareOpBitOffset),
			CompareOpEqual = (CompareOp::Equal << _CompareOpBitOffset),
			CompareOpLessOrEqual = (CompareOp::LessOrEqual << _CompareOpBitOffset),
			CompareOpGreater = (CompareOp::Greater << _CompareOpBitOffset),
			CompareOpNotEqual = (CompareOp::NotEqual << _CompareOpBitOffset),
			CompareOpGreaterOrEqual = (CompareOp::GreaterOrEqual << _CompareOpBitOffset),
			CompareOpAlways = (CompareOp::Always << _CompareOpBitOffset),

			_OptionalValueBit = (1u << 31u),
			EnableBit = _OptionalValueBit,
		};

		enum class DepthStencilStateFlags : uint32_t
		{
			DepthTestBit = (0x1 << 0),
			DepthWriteBit = (0x1 << 1),
			DepthBoundsTestBit = (0x1 << 2),
			StencilTestBit = (0x1 << 3),

			_CompareOpBitOffset = 4,
			CompareOpNever = (CompareOp::Never << _CompareOpBitOffset),
			CompareOpLess = (CompareOp::Less << _CompareOpBitOffset),
			CompareOpEqual = (CompareOp::Equal << _CompareOpBitOffset),
			CompareOpLessOrEqual = (CompareOp::LessOrEqual << _CompareOpBitOffset),
			CompareOpGreater = (CompareOp::Greater << _CompareOpBitOffset),
			CompareOpNotEqual = (CompareOp::NotEqual << _CompareOpBitOffset),
			CompareOpGreaterOrEqual = (CompareOp::GreaterOrEqual << _CompareOpBitOffset),
			CompareOpAlways = (CompareOp::Always << _CompareOpBitOffset),

			_OptionalValueBit = (1u << 31u),
		};
	}

	THAT_DECLARE_ENUM_CLASS_OPERATORS(impl::StencilOpStateFlags)
	THAT_DECLARE_ENUM_CLASS_OPERATORS(impl::DepthStencilStateFlags)

	struct StencilOpState
	{
		using Flags = impl::StencilOpStateFlags;

		Flags flags = {};
		uint32_t compare_mask = {};
		uint32_t write_mask = {};
		uint32_t reference = {};

		constexpr bool hasValue() const noexcept
		{
			return bool(flags & Flags::_OptionalValueBit);
		}

		constexpr void setOptionalOn() noexcept
		{
			flags |= Flags::_OptionalValueBit;
		}

		constexpr VkStencilOpState link() const
		{
			VkStencilOpState res{
				.failOp = static_cast<VkStencilOp>((flags >> Flags::_FailOpBitOffset) & std::bitMask(uint32_t(StencilOp::_BitCount))),
				.passOp = static_cast<VkStencilOp>((flags >> Flags::_PassOpBitOffset) & std::bitMask(uint32_t(StencilOp::_BitCount))),
				.depthFailOp = static_cast<VkStencilOp>((flags >> Flags::_DepthFailOpBitOffset) & std::bitMask(uint32_t(StencilOp::_BitCount))),
				.compareOp = static_cast<VkCompareOp>((flags >> Flags::_CompareOpBitOffset) & std::bitMask(uint32_t(CompareOp::_BitCount))),
				.compareMask = compare_mask,
				.writeMask = write_mask,
				.reference = reference,
			};
			return res;
		}

		constexpr VkStencilOpState linkOrDefault() const
		{
			return hasValue() ? link() : VkStencilOpState{
				.failOp = VK_STENCIL_OP_KEEP,
				.passOp = VK_STENCIL_OP_KEEP,
				.depthFailOp = VK_STENCIL_OP_KEEP,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.compareMask = 0x00,
				.writeMask = 0x00,
				.reference = 0x00,
			};
		}

		constexpr bool operator!=(StencilOpState const& other) const noexcept
		{
			bool res = false;
			res |= flags != other.flags;
			res |= compare_mask != other.compare_mask;
			res |= write_mask != other.write_mask;
			res |= reference != other.reference;
			return res;
		}
	};

	struct DepthStencilState
	{
		using Flags = impl::DepthStencilStateFlags;
		
		Dyn<Flags> flags = {};
		Dyn<Vector2f> depth_bounds = {};
		Dyn<StencilOpState> front_stencil = {};
		Dyn<StencilOpState> back_stencil = {};

		struct Static
		{
			Flags flags = Flags(0);
			Vector2f depth_bounds = {};
			StencilOpState front = {};
			StencilOpState back = {};

			constexpr bool hasValue() const noexcept
			{
				return bool(flags & Flags::_OptionalValueBit);
			}

			constexpr void setOptionalOn() noexcept
			{
				flags |= Flags::_OptionalValueBit;
			}

			bool operator!=(Static const& other) const noexcept
			{
				bool res = false;
				res |= flags != other.flags;
				res |= depth_bounds != other.depth_bounds;
				res |= front != other.front;
				res |= back != other.back;
				return res;
			}

			VkPipelineDepthStencilStateCreateInfo link() const
			{
				VkPipelineDepthStencilStateCreateInfo res{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.depthTestEnable = bool(flags & Flags::DepthTestBit) ? VK_TRUE : VK_FALSE,
					.depthWriteEnable = bool(flags & Flags::DepthWriteBit) ? VK_TRUE : VK_FALSE,
					.depthCompareOp = static_cast<VkCompareOp>((flags >> Flags::_CompareOpBitOffset) & std::bitMask(uint32_t(CompareOp::_BitCount))),
					.depthBoundsTestEnable = bool(flags & Flags::DepthBoundsTestBit) ? VK_TRUE : VK_FALSE,
					.stencilTestEnable = bool(flags & Flags::StencilTestBit) ? VK_TRUE : VK_FALSE,
					.front = front.linkOrDefault(),
					.back = back.linkOrDefault(),
					.minDepthBounds = depth_bounds[0],
					.maxDepthBounds = depth_bounds[1],
				};
				return res;
			}
		};

		Static eval() const
		{
			Static res = {
				.flags = flags.valueOr(Flags(0)) | Flags::_OptionalValueBit,
				.depth_bounds = depth_bounds.valueOr(Vector2f(0, 1)),
			};
			if (front_stencil) res.front = front_stencil.value();
			if (back_stencil) res.back = back_stencil.value();
			return res;
		}
	};

	

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
			DepthStencilState::Static depth_stencil = {};
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
		DepthStencilState::Static _depth_stencil = {};
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

		using DepthStencilState = DepthStencilState;

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
			std::optional<DepthStencilState> depth_stencil = {};
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
		std::optional<DepthStencilState> _depth_stencil = {};
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
