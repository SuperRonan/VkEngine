#include <vkl/VkObjects/RenderPass.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <cassert>

namespace vkl
{
	VkAttachmentDescriptionFlags AttachmentDescription2::GetVkFlags(Flags flags)
	{
		VkAttachmentDescriptionFlags res = static_cast<VkAttachmentDescriptionFlags>((flags & Flags::FlagsMask) >> FlagsBitOffset);
		return res;
	}

	VkAttachmentLoadOp AttachmentDescription2::GetLoadOp(Flags flags)
	{
		const LoadOpFlags lop = static_cast<LoadOpFlags>((flags & Flags::LoadOpMask) >> FlagsLoadOpBitOffset);
		VkAttachmentLoadOp res = static_cast<VkAttachmentLoadOp>(lop);
		if (lop == LoadOpFlags::None)
		{
			res = VK_ATTACHMENT_LOAD_OP_NONE_KHR;
		}
		return res;
	}

	VkAttachmentStoreOp AttachmentDescription2::GetStoreOp(Flags flags)
	{
		const StoreOpFlags sop = static_cast<StoreOpFlags>((flags & Flags::StoreOpMask) >> FlagsStoreOpBitOffset);
		VkAttachmentStoreOp res = static_cast<VkAttachmentStoreOp>(sop);
		if (sop == StoreOpFlags::None)
		{
			res = VK_ATTACHMENT_STORE_OP_NONE;
		}
		return res;
	}

	VkAttachmentLoadOp AttachmentDescription2::GetStencilLoadOp(Flags flags)
	{
		return GetLoadOp(static_cast<Flags>(flags >> FlagsStencilOpShift));
	}

	VkAttachmentStoreOp AttachmentDescription2::GetStencilStoreOp(Flags flags)
	{
		return GetStoreOp(static_cast<Flags>(flags >> FlagsStencilOpShift));
	}

	AttachmentDescription2::Flags AttachmentDescription2::GetFlag(VkAttachmentLoadOp lop)
	{
		Flags res;
		if (lop <= VK_ATTACHMENT_LOAD_OP_DONT_CARE)
		{
			res = static_cast<Flags>(lop) << FlagsLoadOpBitOffset;
		}
		else
		{
			res = Flags::LoadOpNone;
		}
		return res;
	}
	
	AttachmentDescription2::Flags AttachmentDescription2::GetFlag(VkAttachmentStoreOp sop)
	{
		Flags res;
		if (sop <= VK_ATTACHMENT_STORE_OP_DONT_CARE)
		{
			res = static_cast<Flags>(sop) << FlagsStoreOpBitOffset;
		}
		else
		{
			res = Flags::StoreOpNone;
		}
		return res;
	}
	
	AttachmentDescription2::Flags AttachmentDescription2::GetStencilFlag(VkAttachmentLoadOp lop)
	{
		return GetFlag(lop) << FlagsStencilOpShift;
	}
	
	AttachmentDescription2::Flags AttachmentDescription2::GetStencilFlag(VkAttachmentStoreOp sop)
	{
		return GetFlag(sop) << FlagsStencilOpShift;
	}


	AttachmentDescription2 AttachmentDescription2::MakeFrom(Dyn<Flags> const& flags, std::shared_ptr<ImageView> const& view)
	{
		AttachmentDescription2 res{
			.flags = flags,
		};
		if (view)
		{
			res.format = view->format();
			res.samples = view->sampleCount();
		}
		return res;
	}

	AttachmentDescription2 AttachmentDescription2::MakeFrom(Dyn<Flags> const& flags, ImageView const& view)
	{
		AttachmentDescription2 res{
			.flags = flags,
			.format = view.format(),
			.samples = view.sampleCount(),
		};
		return res;
	}

	AttachmentDescription2 AttachmentDescription2::MakeFrom(Dyn<Flags> && flags, std::shared_ptr<ImageView> const& view)
	{
		AttachmentDescription2 res{
			.flags = std::move(flags),
		};
		if (view)
		{
			res.format = view->format();
			res.samples = view->sampleCount();
		}
		return res;
	}

	AttachmentDescription2 AttachmentDescription2::MakeFrom(Dyn<Flags> && flags, ImageView const& view)
	{
		AttachmentDescription2 res{
			.flags = std::move(flags),
			.format = view.format(),
			.samples = view.sampleCount(),
		};
		return res;
	}

	RenderPassInstance::~RenderPassInstance()
	{
		if (_handle)
		{
			destroy();
		}
	}

	void RenderPassInstance::setVulkanName()
	{
		if (!name().empty())
		{
			VkDebugUtilsObjectNameInfoEXT vk_name = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_RENDER_PASS,
				.objectHandle = (uint64_t)_handle,
				.pObjectName = name().c_str(),
			};
			_app->nameVkObjectIFP(vk_name);
		}
	}

	void RenderPassInstance::create(VkRenderPassCreateInfo2 const& ci)
	{
		assert(_handle == VK_NULL_HANDLE);
		VK_CHECK(vkCreateRenderPass2(_app->device(), &ci, nullptr, &_handle), "Failed to create a render pass.");
		setVulkanName();
	}

	void RenderPassInstance::destroy()
	{
		assert(_handle);
		vkDestroyRenderPass(_app->device(), _handle, nullptr);
		_handle = VK_NULL_HANDLE;
	}

	void RenderPassInstance::link()
	{
		const auto linkPtrAsIndex = [&] <class C> (const C * & ptr, const C * base)
		{
			uintptr_t & as_index = reinterpret_cast<uintptr_t&>(ptr);
			if (as_index == uintptr_t(-1))
			{
				ptr = nullptr;
			}
			else
			{
				ptr = base + as_index;
			}
		};

		const auto linkAttachments = [&](const VkAttachmentReference2*& ptr)
		{
			linkPtrAsIndex(ptr, _attachement_references.data());
		};

		const auto linkPreserveAttachments = [&](const uint32_t*& ptr)
		{
			linkPtrAsIndex(ptr, _preserve_attachments_indices.data());
		};

		for (uint32_t s = 0; s < _subpasses.size32(); ++s)
		{
			VkSubpassDescription2 & sd = _subpasses[s];
			linkAttachments(sd.pInputAttachments);
			linkAttachments(sd.pColorAttachments);
			linkAttachments(sd.pResolveAttachments);
			linkAttachments(sd.pDepthStencilAttachment);
			linkPreserveAttachments(sd.pPreserveAttachments);

			SubPassInfo const& info = _subpass_info[s];
			pNextChain chain = &sd;
			if (info.shading_rate_index != uint32_t(-1))
			{
				VkFragmentShadingRateAttachmentInfoKHR & fsrai = _subpass_shading_rate[info.shading_rate_index];
				chain += &fsrai;
				linkAttachments(fsrai.pFragmentShadingRateAttachment);
			}
			if (info.inline_multisampling_index != uint32_t(-1))
			{
				VkMultisampledRenderToSingleSampledInfoEXT & mrsi = _subpass_inline_multisampling[info.inline_multisampling_index];
				chain += &mrsi;
			}
			if (info.creation_feedback_index != uint32_t(-1))
			{
				SubpassCreationFeedback & scf = _subpass_creation_feedback[info.creation_feedback_index];
				if (scf.control.sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT)
				{
					chain += &scf.control;
				}
				if (scf.ci.sType == VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT)
				{
					chain += &scf.ci;
					scf.ci.pSubpassFeedback = &scf.info;
				}
			}

			chain += nullptr;
		}
	}

	void RenderPassInstance::deduceUsages()
	{
		_attachments_usages.resize(_attachement_descriptors.size());
		_attachments_usages_per_subpass.resize(_attachement_descriptors.size() * _subpasses.size());

		const auto writeUsageIFP = [&](const VkAttachmentReference2* attachments, uint32_t count, AttachmentSubpassUsage::Usage usage, uint32_t subpass)
		{
			if (!!attachments)
			{
				for (uint32_t i = 0; i < count; ++i)
				{
					const uint32_t index = attachments[i].attachment;
					if (index != VK_ATTACHMENT_UNUSED)
					{
						_attachments_usages_per_subpass[subpass * _attachement_descriptors.size32() + index].usage |= usage;
					}
				}
			}
		};

		const auto writePreserveUsageIFP = [&](const uint32_t* indices, uint32_t count, uint32_t subpass)
		{
			if (!!indices)
			{
				for (uint32_t i = 0; i < count; ++i)
				{
					const uint32_t index = indices[i];
					if (index != VK_ATTACHMENT_UNUSED)
					{
						_attachments_usages_per_subpass[indexAttachmentUsagePerSubpass(subpass, index)].usage |= AttachmentSubpassUsage::Usage::Preserve;
					}
				}
			}
		};

		for (uint32_t s = 0; s < _subpasses.size32(); ++s)
		{
			const VkSubpassDescription2 & desc = _subpasses[s];
			writeUsageIFP(desc.pInputAttachments, desc.inputAttachmentCount, AttachmentSubpassUsage::Usage::Input, s);
			writeUsageIFP(desc.pColorAttachments, desc.colorAttachmentCount, AttachmentSubpassUsage::Usage::Color, s);
			if (!!desc.pResolveAttachments)
			{
				for (uint32_t i = 0; i < desc.colorAttachmentCount; ++i)
				{
					VkAttachmentReference2 const& ref = desc.pResolveAttachments[i];
					if (ref.attachment != VK_ATTACHMENT_UNUSED)
					{
						_attachments_usages_per_subpass[indexAttachmentUsagePerSubpass(s, ref.attachment)].usage |= AttachmentSubpassUsage::Usage::ResolveDst;
						_attachments_usages_per_subpass[indexAttachmentUsagePerSubpass(s, desc.pColorAttachments[i].attachment)].usage |= AttachmentSubpassUsage::Usage::ResolveSrc;
					}
				}
			}
			writeUsageIFP(desc.pDepthStencilAttachment, 1, AttachmentSubpassUsage::Usage::DepthStencil, s);
			writePreserveUsageIFP(desc.pPreserveAttachments, desc.preserveAttachmentCount, s);

			const SubPassInfo & info = _subpass_info[s];
			if (info.inline_multisampling_index != uint32_t(-1))
			{
				writeUsageIFP(_subpass_shading_rate[info.inline_multisampling_index].pFragmentShadingRateAttachment, 1, AttachmentSubpassUsage::Usage::ShadingRate, s);
			}
		}
	}

	void RenderPassInstance::prepareSynch()
	{
		_attachments_usages.resize(_attachement_descriptors.size());
		for (uint32_t i = 0; i < _attachement_descriptors.size32(); ++i)
		{
			AttachmentUsage & au = _attachments_usages[i];
			au.first_subpass = 0;
			au.last_subpass = 0;
		}

		that::ExtensibleStorage<VkSubpassDependency2> dependecies;

		if (_subpasses.size32() == 1)
		{

			const auto findRef = [&](const VkAttachmentReference2* list, uint32_t count, uint32_t index)
			{
				const VkAttachmentReference2* res = nullptr;
				if (list)
				{
					for (uint32_t i = 0; i < count; ++i)
					{
						if (list[i].attachment == index)
						{
							res = list + i;
							break;
						}
					}
				}
				return res;
			};
			
			const VkSubpassDescription2 & subpass = _subpasses[0];

			for (uint32_t i = 0; i < _attachement_descriptors.size32(); ++i)
			{
				const AttachmentSubpassUsage::Usage usage = _attachments_usages_per_subpass[indexAttachmentUsagePerSubpass(0, i)].usage;
				VkAttachmentDescription2 & desc = _attachement_descriptors[i];
				AttachmentUsage & global_usage = _attachments_usages[i];
				const VkImageAspectFlags aspect = getImageAspectFromFormat(desc.format);
				const bool is_color = aspect & VK_IMAGE_ASPECT_COLOR_BIT;
				const bool is_depth = aspect & VK_IMAGE_ASPECT_DEPTH_BIT;
				const bool is_stencil = aspect & VK_IMAGE_ASPECT_STENCIL_BIT;

				global_usage.flags = static_cast<AttachmentDescription2::Flags>(desc.flags);

				VkAccessFlags access = VK_ACCESS_NONE;
				VkPipelineStageFlags initial_stage = VK_PIPELINE_STAGE_NONE;
				VkPipelineStageFlags final_stage = VK_PIPELINE_STAGE_NONE;
				VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkImageUsageFlags vk_image_usage = 0;

				bool deduce_from_op = true;

				if ((usage & AttachmentSubpassUsage::Usage::Input) != AttachmentSubpassUsage::Usage::None)
				{
					// It does not make sense to have an input attachment at subpass 0
					const VkAttachmentReference2* ref = findRef(subpass.pInputAttachments, subpass.inputAttachmentCount, i);
					vk_image_usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
					if (ref)
					{
						layout = ref->layout;
					}
					else
					{
						assert(false);
					}
				}
				if ((usage & AttachmentSubpassUsage::Usage::Color) != AttachmentSubpassUsage::Usage::None)
				{
					const VkAttachmentReference2* ref = findRef(subpass.pColorAttachments, subpass.colorAttachmentCount, i);
					vk_image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					if (ref)
					{
						layout = ref->layout;
					}
					else
					{
						assert(false);
					}
				}
				if ((usage & AttachmentSubpassUsage::Usage::DepthStencil) != AttachmentSubpassUsage::Usage::None)
				{
					vk_image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					if (subpass.pDepthStencilAttachment)
					{
						layout = subpass.pDepthStencilAttachment->layout;
					}
					else
					{
						assert(false);
					}
				}
				if ((usage & AttachmentSubpassUsage::Usage::ResolveDst) != AttachmentSubpassUsage::Usage::None)
				{
					const VkAttachmentReference2* ref = findRef(subpass.pResolveAttachments, subpass.colorAttachmentCount, i);
					vk_image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					//vk_image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; vkCmdResolveImage requires both

					if (ref)
					{
						layout = ref->layout;
					}
					else
					{
						assert(false);
					}
				}
				if ((usage & AttachmentSubpassUsage::Usage::Preserve) != AttachmentSubpassUsage::Usage::None)
				{
					// It does not make sense to have preserve attachments with only one subpass
				}
				if ((usage & AttachmentSubpassUsage::Usage::ShadingRate) != AttachmentSubpassUsage::Usage::None)
				{
					assert(!!_subpass_shading_rate[_subpass_info[0].shading_rate_index].pFragmentShadingRateAttachment);
					layout = _subpass_shading_rate[_subpass_info[0].shading_rate_index].pFragmentShadingRateAttachment->layout;
					initial_stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
					final_stage = initial_stage;
					access = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
					deduce_from_op = false;
					vk_image_usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
				}

				if (deduce_from_op)
				{
					if (is_color)
					{
						initial_stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						final_stage = initial_stage;
						switch (desc.loadOp)
						{
						case VK_ATTACHMENT_LOAD_OP_LOAD:
							access |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
						break;
						case VK_ATTACHMENT_LOAD_OP_CLEAR:
						case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
							access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						break;
						}
						switch (desc.storeOp)
						{
						case VK_ATTACHMENT_STORE_OP_STORE:
						case VK_ATTACHMENT_STORE_OP_DONT_CARE:
							access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						break;
						case VK_ATTACHMENT_STORE_OP_NONE:
							// weird but the spec says so
							access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						break;
						}
					}
					else
					{
						initial_stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
						final_stage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					
						bool depth_read_only = false;
						bool stencil_read_only = false;
						switch (layout)
						{
						case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
							depth_read_only = true;
							stencil_read_only = true;
						break;
						case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
							depth_read_only = true;
							stencil_read_only = false;
						break;
						case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
							depth_read_only = false;
							stencil_read_only = true;
						break;
						case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
							depth_read_only = false;
							stencil_read_only = false;
						break;
						}
				
						switch (desc.loadOp)
						{
						case VK_ATTACHMENT_LOAD_OP_LOAD:
							access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
							break;
						case VK_ATTACHMENT_LOAD_OP_CLEAR:
						case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
							access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
							break;
						}
						switch (desc.storeOp)
						{
						case VK_ATTACHMENT_STORE_OP_STORE:
						case VK_ATTACHMENT_STORE_OP_DONT_CARE:
							access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
							break;
						case VK_ATTACHMENT_STORE_OP_NONE:
							// weird but the spec says so
							access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
							break;
						}
					}
				}
				
				
				desc.initialLayout = layout;
				desc.finalLayout = layout;
				global_usage.initial_access = access;
				global_usage.final_access = access;
				global_usage.initial_stage = initial_stage;
				global_usage.final_stage = final_stage;
				global_usage.usage |= vk_image_usage;

				global_usage.flags |= AttachmentDescription2::GetFlag(desc.loadOp);
				global_usage.flags |= AttachmentDescription2::GetFlag(desc.storeOp);
				if (is_stencil)
				{
					global_usage.flags |= AttachmentDescription2::GetStencilFlag(desc.stencilLoadOp);
					global_usage.flags |= AttachmentDescription2::GetStencilFlag(desc.stencilStoreOp);
				}
			}

			//VkSubpassDependency2 dep{
			//	.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
			//	.pNext = nullptr,
			//	.srcSubpass = 0,
			//	.dstSubpass = 0,
			//	.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			//	.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			//	.srcAccessMask = 0,
			//	.dstAccessMask = 0,
			//	.dependencyFlags = 0,
			//	.viewOffset = 0,
			//};
			//if (subpass.viewMask)
			//{
			//	dep.dependencyFlags |= VK_DEPENDENCY_VIEW_LOCAL_BIT;
			//}
			//dependecies.pushBack(&dep, 1);
		}
		else
		{
			NOT_YET_IMPLEMENTED;
			for (uint32_t s = 0; s < _subpasses.size32(); ++s)
			{
				const uint32_t prev_subpass = (s == 0) ? VK_SUBPASS_EXTERNAL : s - 1;
				VkSubpassDescription2 const& subpass = _subpasses[s];

				VkSubpassDependency2 dep{
					.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
					.pNext = nullptr,
					.srcSubpass = prev_subpass,
					.dstSubpass = s,
					.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
					.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
					.srcAccessMask = VK_ACCESS_2_NONE,
					.dstAccessMask = VK_ACCESS_2_NONE,
					.dependencyFlags = 0,
					.viewOffset = 0,
				};

				for (uint32_t i = 0; i < _attachement_descriptors.size32(); ++i)
				{

				}
			}
		}

		_dependencies = std::move(dependecies.getStorage());
	}

	void RenderPassInstance::unpack(CreateInfo const& ci)
	{
		_subpasses.resize(ci.subpasses.size());
		_subpass_info.resize(_subpasses.size());
		that::ExtensibleStorage<VkFragmentShadingRateAttachmentInfoKHR> subpass_shading_rate;
		that::ExtensibleStorage<VkMultisampledRenderToSingleSampledInfoEXT> subpass_inline_multisampling;
		that::ExtensibleStorage<SubpassCreationFeedback> subpass_creation_feedback;
		for (uint32_t s = 0; s < _subpasses.size32(); ++s)
		{
			SubPassCreateInfo const& sci = ci.subpasses[s];
			_subpasses[s] = sci.desc;

			if (sci.shading_rate.sType == VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR)
			{
				_subpass_info[s].shading_rate_index = subpass_shading_rate.pushBack(&sci.shading_rate, 1);
			}

			if (sci.inline_multisampling.sType == VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT)
			{
				_subpass_info[s].inline_multisampling_index = subpass_inline_multisampling.pushBack(&sci.inline_multisampling, 1);
			}

			if (sci.disallow_merging || ci.query_creation_feedback)
			{
				_subpass_info[s].creation_feedback_index = subpass_creation_feedback.pushBack<SubpassCreationFeedback>(nullptr, 1);
				SubpassCreationFeedback* scf = subpass_creation_feedback.data() + _subpass_info[s].creation_feedback_index;
				std::memset(scf, 0, sizeof(SubpassCreationFeedback));
				if (sci.disallow_merging)
				{
					scf->control = VkRenderPassCreationControlEXT{
						.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT,
						.pNext = nullptr,
						.disallowMerging = VK_TRUE,
					};
				}
				if (ci.query_creation_feedback)
				{
					scf->ci = VkRenderPassSubpassFeedbackCreateInfoEXT{
						.sType = VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT,
						.pNext = nullptr,
						.pSubpassFeedback = nullptr,
					};
				}
			}
		}
		_subpass_shading_rate = std::move(subpass_shading_rate.getStorage());
		_subpass_inline_multisampling = std::move(subpass_inline_multisampling.getStorage());
		_subpass_creation_feedback = std::move(subpass_creation_feedback.getStorage());
	}

	void RenderPassInstance::construct(CreateInfo const& ci)
	{
		unpack(ci);
		
		link();
		
		deduceUsages();

		prepareSynch();

		_vk_ci2 = VkRenderPassCreateInfo2{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = _attachement_descriptors.size32(),
			.pAttachments = _attachement_descriptors.data(),
			.subpassCount = _subpasses.size32(),
			.pSubpasses = _subpasses.data(),
			.dependencyCount = _dependencies.size32(),
			.pDependencies = _dependencies.data(),
			.correlatedViewMaskCount = 0,
			.pCorrelatedViewMasks = nullptr,
		};
		
		pNextChain chain = &_vk_ci2;

		if (ci.disallow_merging)
		{
			_creation_control = VkRenderPassCreationControlEXT{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT,
				.pNext = nullptr,
				.disallowMerging = VK_TRUE,
			};
			chain += &_creation_control;
		}

		create(_vk_ci2);

	}

	RenderPassInstance::RenderPassInstance(CreateInfo const& ci) :
		AbstractInstance(ci.app, ci.name),
		_attachement_descriptors(ci.attachement_descriptors),
		_attachement_references(ci.attachement_references),
		_preserve_attachments_indices(ci.preserve_attachments_indices)
	{
		construct(ci);
	}

	RenderPassInstance::RenderPassInstance(CreateInfo && ci) :
		AbstractInstance(ci.app, ci.name),
		_attachement_descriptors(std::move(ci.attachement_descriptors)),
		_attachement_references(std::move(ci.attachement_references)),
		_preserve_attachments_indices(std::move(ci.preserve_attachments_indices))
	{
		construct(ci);
	}









	RenderPass::RenderPass(CreateInfo const& ci) :
		ParentType(ci.app, ci.name, ci.hold_instance),
		_mode(ci.mode),
		_attachments(ci.attachments),
		_subpasses(ci.subpasses)
	{
		if (ci.create_on_construct)
		{
			createInstance();
		}
	}

	RenderPass::RenderPass(SinglePassCreateInfo const& ci):
		RenderPass([&]() -> CreateInfo
		{
			CreateInfo res{
				.app = ci.app,
				.name = ci.name,
				.mode = ci.mode,
				.subpasses = {SubPassDescription2{
					.view_mask = ci.view_mask,
					.read_only_depth = ci.read_only_depth,
					.read_only_stencil = ci.read_only_stencil,
					.auto_preserve_all_other_attachments = false,
					.shading_rate_texel_size = ci.fragment_shading_rate_texel_size,
					.inline_multisampling = ci.inline_multisampling,
				}},
				.create_on_construct = ci.create_on_construct,
				.hold_instance = ci.hold_instance,
			};
			SubPassDescription2& subpass = res.subpasses.front();
			that::ExS<AttachmentDescription2> attachments;

			const uint32_t color_base = attachments.pushBack(ci.colors.data(), ci.colors.size());
			subpass.colors.resize(ci.colors.size());
			if (ci.depth_stencil)
			{
				const uint32_t depth_stencil_base = attachments.pushBack(&ci.depth_stencil, 1);
				subpass.depth_stencil = AttachmentReference2{
					.index = depth_stencil_base,
				};
			}
			uint32_t resolve_base = 0;
			if (ci.resolve)
			{
				resolve_base = attachments.pushBack(ci.resolve.data(), ci.resolve.size());
				subpass.resolves.resize(ci.colors.size());
			}
			if (ci.fragment_shading_rate)
			{
				const uint32_t fragment_shading_rate_base = attachments.pushBack(&ci.fragment_shading_rate, 1);
				subpass.fragment_shading_rate = AttachmentReference2{
					.index = fragment_shading_rate_base,
				};
			}
			
			for (uint32_t i = 0; i < ci.colors.size32(); ++i)
			{
				subpass.colors[i].index = color_base + i;
				if (ci.resolve)
				{
					if (i < ci.resolve.size32())
					{
						if (!res.attachments[resolve_base + i].flags)
						{
							res.attachments[resolve_base + i].flags = AttachmentDescription2::Flags::OverWrite;
						}
						subpass.resolves[i].index = resolve_base + i;
					}
				}
			}

			res.attachments = std::move(attachments.getStorage());

			return res;
		}())
	{}

	RenderPass::~RenderPass()
	{
		
	}

	void RenderPass::createInstance()
	{
		assert(!_inst);

		const bool can_disallow_merge = application()->availableFeatures().subpass_merge_feedback.subpassMergeFeedback;

		MyVector<VkAttachmentDescription2> vk_attachements(_attachments.size());
		for (size_t i = 0; i < vk_attachements.size(); ++i)
		{
			AttachmentDescription2 const& desc = _attachments[i];
			const AttachmentDescription2::Flags flags = desc.flags.valueOr(AttachmentDescription2::Flags::None);
			assert(desc.format.hasValue());
			vk_attachements[i] = VkAttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = AttachmentDescription2::GetVkFlags(flags),
				.format = desc.format.value(),
				.samples = desc.samples.valueOr(VK_SAMPLE_COUNT_1_BIT),
				.loadOp = AttachmentDescription2::GetLoadOp(flags),
				.storeOp = AttachmentDescription2::GetStoreOp(flags),
				.stencilLoadOp = AttachmentDescription2::GetStencilLoadOp(flags),
				.stencilStoreOp = AttachmentDescription2::GetStencilStoreOp(flags),
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
		}

		that::ExtensibleStorage<VkAttachmentReference2> vk_attachments_refs;
		MyVector<RenderPassInstance::SubPassCreateInfo> vk_subpasses(_subpasses.size());
		
		MyVector<bool> attachment_is_used;
		MyVector<uint32_t> preserve_attachments_indices;
		
		for (uint32_t s = 0; s < vk_subpasses.size32(); ++s)
		{
			const SubPassDescription2 & desc = _subpasses[s];
			VkSubpassDescription2 & vk_desc = vk_subpasses[s].desc;
			if (desc.auto_preserve_all_other_attachments)
			{
				attachment_is_used.resize(_attachments.size());
				std::fill_n(attachment_is_used.begin(), _attachments.size32(), false);
			}
			vk_desc = VkSubpassDescription2{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = desc.flags,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.viewMask = desc.view_mask,
			};

			const auto pushAttachments = [&](
				const AttachmentReference2* src, 
				uint32_t count, 
				const VkAttachmentReference2*& dst, 
				uint32_t * dst_count,
				VkImageLayout optimal_layout,
				VkImageUsageFlags usage
			) {
				const VkImageLayout layout = application()->options().getLayout(optimal_layout, usage);
				if (dst_count)
				{
					*dst_count = count;
				}
				uintptr_t & dst_ptr_index = reinterpret_cast<uintptr_t&>(dst);
				bool emit_attachments = src && count > 0;
				if (emit_attachments && count == 1 && src[0].index == VK_ATTACHMENT_UNUSED && !dst_count)
				{
					emit_attachments = false;
				}
				if (emit_attachments)
				{
					const uint32_t base = vk_attachments_refs.pushBack<VkAttachmentReference2>(nullptr, count);
					dst_ptr_index = base;
					for (uint32_t i = 0; i < count; ++i)
					{
						const AttachmentReference2 & src_ref = src[i];
						VkAttachmentReference2 & dst_ref = vk_attachments_refs.data()[base + i];
						dst_ref = VkAttachmentReference2{
							.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
							.pNext = nullptr,
							.attachment = src_ref.index,
							.layout = layout,
							.aspectMask = VK_IMAGE_ASPECT_NONE,
						};
						
						if (src_ref.index != VK_ATTACHMENT_UNUSED)
						{
							const VkAttachmentDescription2 & src_desc = vk_attachements[src_ref.index];
							dst_ref.aspectMask = getImageAspectFromFormat(src_desc.format);
							dst_ref.aspectMask &= src_ref.aspect;
							if (desc.auto_preserve_all_other_attachments)
							{
								attachment_is_used[dst_ref.attachment] = true;
							}
						}
						else
						{
							dst_ref.layout = VK_IMAGE_LAYOUT_UNDEFINED;
						}
					}
				}
				else
				{
					dst_ptr_index = uintptr_t(-1);
				}
			};
			
			// Can an attachment have multiple usages in a subpass?

			pushAttachments(desc.inputs.data(), desc.inputs.size32(), vk_desc.pInputAttachments, &vk_desc.inputAttachmentCount, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
			pushAttachments(desc.colors.data(), desc.colors.size32(), vk_desc.pColorAttachments, &vk_desc.colorAttachmentCount, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			assert(desc.resolves.size() == 0 || desc.resolves.size() == desc.colors.size());
			pushAttachments(desc.resolves.data(), desc.resolves.size32(), vk_desc.pResolveAttachments, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
			VkImageLayout depth_stencil_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			// TODO check for separate depth stencil aspects
			if (desc.read_only_depth)
			{
				if (desc.read_only_stencil)
				{
					depth_stencil_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else
				{
					depth_stencil_layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
			}
			else
			{
				if (desc.read_only_stencil)
				{
					depth_stencil_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
				else
				{
					depth_stencil_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
			}
			pushAttachments(&desc.depth_stencil, 1, vk_desc.pDepthStencilAttachment, nullptr, depth_stencil_layout, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			vk_subpasses[s].shading_rate.sType = VK_STRUCTURE_TYPE_MAX_ENUM;
			vk_subpasses[s].inline_multisampling.sType = VK_STRUCTURE_TYPE_MAX_ENUM;

			if (application()->availableFeatures().fragment_shading_rate_khr.attachmentFragmentShadingRate)
			{
				const bool has_attachment = desc.fragment_shading_rate.index != VK_ATTACHMENT_UNUSED;
				if (has_attachment)
				{
					vk_subpasses[s].shading_rate = VkFragmentShadingRateAttachmentInfoKHR{
						.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR,
						.pNext = nullptr,
						.shadingRateAttachmentTexelSize = desc.shading_rate_texel_size.valueOr(VkExtent2D{.width = 1, .height = 1}),
					};
					pushAttachments(&desc.fragment_shading_rate, 1, vk_subpasses[s].shading_rate.pFragmentShadingRateAttachment, nullptr, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR);
				}
			}
			
			if (application()->availableFeatures().multisampled_render_to_single_sampled_ext.multisampledRenderToSingleSampled)
			{
				if (desc.inline_multisampling.hasValue())
				{
					const VkSampleCountFlagBits samples = desc.inline_multisampling.value();
					vk_subpasses[s].inline_multisampling = VkMultisampledRenderToSingleSampledInfoEXT{
						.sType = VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT,
						.pNext = nullptr,
						.multisampledRenderToSingleSampledEnable = (samples == VK_SAMPLE_COUNT_1_BIT) ? VK_FALSE : VK_TRUE,
						.rasterizationSamples = samples,
					};
				}
			}


			{
				uintptr_t & preserve_ptr_index = reinterpret_cast<uintptr_t&>(vk_desc.pPreserveAttachments);
				const uint32_t preserve_index_base = preserve_attachments_indices.size();
				if (desc.auto_preserve_all_other_attachments)
				{
					for (uint32_t i = 0; i < _attachments.size32(); ++i)
					{
						if (!attachment_is_used[i])
						{
							preserve_attachments_indices.push_back(i);
						}
					}
				}
				else if(desc.explicit_preserve_attachments)
				{
					preserve_attachments_indices.resize(preserve_attachments_indices.size() + desc.explicit_preserve_attachments.size());
					std::memcpy(preserve_attachments_indices.data() + preserve_index_base, desc.explicit_preserve_attachments.data(), desc.explicit_preserve_attachments.size());
				}
				vk_desc.preserveAttachmentCount = preserve_attachments_indices.size32() - preserve_index_base;
				if (vk_desc.preserveAttachmentCount > 0)
				{
					preserve_ptr_index = preserve_index_base;
				}
				else
				{
					preserve_ptr_index = uintptr_t(-1);
				}
			}

			vk_subpasses[s].disallow_merging = desc.disallow_merging && can_disallow_merge;
		}

		bool create_vk_render_pass = _mode == Mode::RenderPass;
		if (_mode == Mode::Automatic)
		{
			create_vk_render_pass = !application()->options().prefer_render_pass_with_dynamic_rendering;
		}

		_inst = std::make_shared<RenderPassInstance>(RenderPassInstance::CI{
			.app = application(),
			.name = name(),
			.create_vk_render_pass = create_vk_render_pass,
			.query_creation_feedback = application()->options().query_render_pass_creation_feedback,
			.disallow_merging = application()->options().render_pass_disallow_merging,
			.attachement_descriptors = std::move(vk_attachements),
			.attachement_references = std::move(vk_attachments_refs.getStorage()),
			.preserve_attachments_indices = std::move(preserve_attachments_indices),
			.subpasses = vk_subpasses,
		});
	}

	bool RenderPass::updateResources(UpdateContext& ctx)
	{
		bool res = false;

		if (checkHoldInstance())
		{
			if (_inst)
			{
				if (_attachments.size() == _inst->_attachement_descriptors.size())
				{
					for (uint32_t i = 0; i < _attachments.size32(); ++i)
					{
						AttachmentDescription2 const& desc = _attachments[i];
						const VkFormat new_format = desc.format.value();
						if (new_format != _inst->_attachement_descriptors[i].format)
						{
							res = true;
							break;
						}
						const VkSampleCountFlagBits new_samples = desc.samples.valueOr(VK_SAMPLE_COUNT_1_BIT);
						if (new_samples != _inst->_attachement_descriptors[i].samples)
						{
							res = true;
							break;
						}
						const AttachmentDescription2::Flags new_flags = desc.flags.valueOr(AttachmentDescription2::Flags::None);
						if (new_flags != _inst->_attachments_usages[i].flags)
						{
							res = true;
							break;
						}
					}
				}
				else
				{
					res = true;
				}
				
				if (!res)
				{
					if (_subpasses.size() == _inst->_subpasses.size())
					{
						for (uint32_t s = 0; s < _subpasses.size32(); ++s)
						{
							SubPassDescription2 const& desc = _subpasses[s];
							if (desc.fragment_shading_rate.index != VK_ATTACHMENT_UNUSED)
							{
								if (desc.shading_rate_texel_size.hasValue())
								{
									const VkExtent2D new_extent = desc.shading_rate_texel_size.value();
									const pNextChain chain = _inst->_subpasses.data() + s;
									if (const VkStruct* candidate = chain.find(VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR))
									{
										const VkFragmentShadingRateAttachmentInfoKHR * value = reinterpret_cast<const VkFragmentShadingRateAttachmentInfoKHR*>(candidate);
										if (value->shadingRateAttachmentTexelSize != new_extent)
										{
											res = true;
											break;
										}
									}
								}
							}
							
							if (desc.inline_multisampling.hasValue())
							{
								const VkSampleCountFlagBits new_ms = desc.inline_multisampling.value();
								const pNextChain chain = _inst->_subpasses.data() + s;
								if (const VkStruct* candidate = chain.find(VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT))
								{
									const VkMultisampledRenderToSingleSampledInfoEXT * value = reinterpret_cast<const VkMultisampledRenderToSingleSampledInfoEXT*>(candidate);
									if (value->rasterizationSamples != new_ms)
									{
										res = true;
										break;
									}
								}
							}
						}
					}
					else
					{
						res = false;
					}
				}

				if (res)
				{
					destroyInstanceIFN();
				}
			}

			if (!_inst)
			{
				res = true;
				createInstance();
			}
		}

		return res;
	}
}