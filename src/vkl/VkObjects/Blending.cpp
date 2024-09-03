#include <vkl/VkObjects/Blending.hpp>

namespace vkl
{

	VkPipelineColorBlendStateCreateInfo& VkPipelineBlendingState::link(const VkPipelineColorBlendAttachmentState* color_attachments, uint32_t count)
	{
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		ci.pNext = nullptr;
		ci.attachmentCount = count;
		ci.pAttachments = color_attachments;

		if (useAdvanced())
		{
			ci.pNext = &advanced;
		}

		return ci;
	}

	void VkPipelineBlendingState::clear()
	{
		std::memset(this, 0, sizeof(VkPipelineBlendingState));
	}

	PipelineBlending PipelineBlending::deduce(VkPipelineBlendingState const& s)
	{
		assert(s.ci.flags <= 1);
		assert(s.ci.logicOpEnable <= 1);
		assert(std::bit_width<impl::PipelineBlendFlagsUint>(static_cast<impl::PipelineBlendFlagsUint>(Convert(s.ci.logicOp))) <= impl::LogicOpBitCount);

		PipelineBlending res;
		res.flags |= s.ci.flags << impl::PipelineBlendCoreFlagsBitOffset;
		res.flags |= s.ci.logicOpEnable << impl::UseLogicOpBitOffset;
		res.flags |= static_cast<Flags>(Convert(s.ci.logicOp)) << impl::LogicOpBitOffset;
		res.blend_constants = Vector4f{
			s.ci.blendConstants[0],
			s.ci.blendConstants[1],
			s.ci.blendConstants[2],
			s.ci.blendConstants[3],
		};

		if (s.useAdvanced())
		{
			assert(s.advanced.srcPremultiplied <= 1);
			assert(s.advanced.dstPremultiplied <= 1);
			assert(s.advanced.blendOverlap <= VK_BLEND_OVERLAP_CONJOINT_EXT);
			res.flags |= Flags::UseAdvancedBlending;
			res.flags |= s.advanced.srcPremultiplied << impl::SrcPremultipliedBitOffset;
			res.flags |= s.advanced.dstPremultiplied << impl::DstPremultipliedBitOffset;
			res.flags |= s.advanced.blendOverlap << impl::BlendOverlapBitOffset;
		}
		
		return res;
	}

	void PipelineBlending::extract(VkPipelineBlendingState& dst) const
	{
		dst.clear();

		dst.ci = VkPipelineColorBlendStateCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = static_cast<VkPipelineColorBlendStateCreateFlags>(detailed.core_flags),
			.logicOpEnable = static_cast<VkBool32>(detailed.use_logic_op),
			.logicOp = Convert(static_cast<LogicOp>(detailed.logic_op)),
			.attachmentCount = 0,
			.pAttachments = nullptr,
			.blendConstants = {blend_constants.r, blend_constants.g, blend_constants.b, blend_constants.a},
		};

		if (detailed.use_advanced_blending)
		{
			dst.advanced = VkPipelineColorBlendAdvancedStateCreateInfoEXT{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT,
				.pNext = nullptr,
				.srcPremultiplied = static_cast<VkBool32>(detailed.src_premultiplied),
				.dstPremultiplied = static_cast<VkBool32>(detailed.dst_premultiplied),
				.blendOverlap = static_cast<VkBlendOverlapEXT>(detailed.blend_overlap),
			};
		}
	}
}