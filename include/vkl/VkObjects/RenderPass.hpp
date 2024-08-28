#pragma once

#include <vkl/App/VkApplication.hpp>
#include "Program.hpp"
#include <vkl/VkObjects/AbstractInstance.hpp>
#include <vkl/Execution/ResourceState.hpp>

#include <that/utils/ExtensibleStorage.hpp>
#include <that/stl_ext/alignment.hpp>
#include <that/utils/EnumClassOperators.hpp>

namespace vkl
{

	struct AttachmentDescription2
	{
		enum class LoadOpFlags : uint32_t
		{
			Load = 0,
			Clear = 1,
			DontCare = 2,
			None = 3,
			MAX_VALUE = None,
		};

		enum class StoreOpFlags : uint32_t
		{
			Store = 0,
			DontCare = 1,
			None = 2,
			MAX_VALUE = None,
		};

		using FlagsUint = uint32_t;

		static constexpr const FlagsUint FlagsBitOffset = 0;
		static constexpr const FlagsUint FlagsBitCount = 1;
		static constexpr const FlagsUint FlagsLoadOpBitOffset = FlagsBitOffset + FlagsBitCount;
		static constexpr const FlagsUint FlagsLoadOpBitCount = std::bit_width(static_cast<uint32_t>(LoadOpFlags::MAX_VALUE));
		static constexpr const FlagsUint FlagsStoreOpBitOffset = FlagsLoadOpBitOffset + FlagsLoadOpBitCount;
		static constexpr const FlagsUint FlagsStoreOpBitCount = std::bit_width(static_cast<uint32_t>(StoreOpFlags::MAX_VALUE));
		static constexpr const FlagsUint FlagsStencilOpBitOffset = FlagsStoreOpBitOffset + FlagsStoreOpBitCount;
		static constexpr const FlagsUint FlagsStencilOpBitCount = FlagsLoadOpBitCount + FlagsStoreOpBitCount;
		static constexpr const FlagsUint FlagsStencilOpShift = FlagsLoadOpBitCount + FlagsStoreOpBitCount;

		enum class Flags : FlagsUint
		{
			None = 0x0,
			
			MayAlias = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT << FlagsBitOffset,
			FlagsMask = std::bitMask(FlagsBitCount) << FlagsBitOffset,

			LoadOpLoad = static_cast<FlagsUint>(LoadOpFlags::Load) << FlagsLoadOpBitOffset,
			LoadOpClear = static_cast<FlagsUint>(LoadOpFlags::Clear) << FlagsLoadOpBitOffset,
			LoadOpDontCare = static_cast<FlagsUint>(LoadOpFlags::DontCare) << FlagsLoadOpBitOffset,
			LoadOpNone = static_cast<FlagsUint>(LoadOpFlags::None) << FlagsLoadOpBitOffset,
			LoadOpMask = std::bitMask(FlagsLoadOpBitCount) << FlagsLoadOpBitOffset,

			StoreOpStore = static_cast<FlagsUint>(StoreOpFlags::Store) << FlagsStoreOpBitOffset,
			StoreOpDontCare = static_cast<FlagsUint>(StoreOpFlags::DontCare) << FlagsStoreOpBitOffset,
			StoreOpNone = static_cast<FlagsUint>(StoreOpFlags::None) << FlagsStoreOpBitOffset,
			StoreOpMask = std::bitMask(FlagsStoreOpBitCount) << FlagsStoreOpBitOffset,

			OverWrite = LoadOpDontCare | StoreOpStore,
			Blend = LoadOpLoad | StoreOpStore,
			Clear = LoadOpClear | StoreOpStore,
			ReadOnly = LoadOpLoad | StoreOpNone,
			DontCare = LoadOpDontCare | StoreOpDontCare,
			OpNone = LoadOpNone | StoreOpNone,


			StencilLoadOpLoad = LoadOpLoad << FlagsStencilOpShift,
			StencilLoadOpClear = LoadOpClear << FlagsStencilOpShift,
			StencilLoadOpDontCare = LoadOpDontCare << FlagsStencilOpShift,
			StencilLoadOpNone = LoadOpNone << FlagsStencilOpShift,
			StencilLoadOpMask = LoadOpMask << FlagsStencilOpShift,
			
			StencilStoreOpStore = StoreOpStore << FlagsStencilOpShift,
			StencilStoreOpDontCare = StoreOpDontCare << FlagsStencilOpShift,
			StencilStoreOpNone = StoreOpNone << FlagsStencilOpShift,
			StencilStoreOpMask = StoreOpMask << FlagsStencilOpShift,
			
			StencilOverWrite = OverWrite << FlagsStencilOpShift,
			StencilBlend = Blend << FlagsStencilOpShift,
			StencilClear = Clear << FlagsStencilOpShift,
			StencilReadOnly = ReadOnly << FlagsStencilOpShift,
			StencilDontCare = DontCare << FlagsStencilOpShift,
			StencilOpNone = OpNone << FlagsStencilOpShift,
		};

		Dyn<Flags> flags = {};
		Dyn<VkFormat> format = {};
		Dyn<VkSampleCountFlagBits> samples = {};

		constexpr operator bool() const
		{
			return format.operator bool();
		}

		static VkAttachmentDescriptionFlags GetVkFlags(Flags flags);
		static VkAttachmentLoadOp GetLoadOp(Flags flags);
		static VkAttachmentStoreOp GetStoreOp(Flags flags);
		static VkAttachmentLoadOp GetStencilLoadOp(Flags flags);
		static VkAttachmentStoreOp GetStencilStoreOp(Flags flags);

		static Flags GetFlag(VkAttachmentLoadOp lop);
		static Flags GetFlag(VkAttachmentStoreOp sop);
		static Flags GetStencilFlag(VkAttachmentLoadOp lop);
		static Flags GetStencilFlag(VkAttachmentStoreOp sop);

		static AttachmentDescription2 MakeFrom(Dyn<Flags> const& flags, std::shared_ptr<ImageView> const& view);
		static AttachmentDescription2 MakeFrom(Dyn<Flags> const& flags, ImageView const& view);
		static AttachmentDescription2 MakeFrom(Dyn<Flags> && flags, std::shared_ptr<ImageView> const& view);
		static AttachmentDescription2 MakeFrom(Dyn<Flags> && flags, ImageView const& view);
	};


	struct AttachmentReference2
	{
		uint32_t index = VK_ATTACHMENT_UNUSED;
		VkImageAspectFlags aspect = VkImageAspectFlags(~VK_IMAGE_ASPECT_NONE);
	};

	struct SubPassDescription2
	{
		VkSubpassDescriptionFlags flags = 0;
		uint32_t view_mask = 0;
		MyVector<AttachmentReference2> inputs = {};
		MyVector<AttachmentReference2> colors = {};
		MyVector<AttachmentReference2> resolves = {};
		AttachmentReference2 depth_stencil = {};
		bool read_only_depth = false;
		bool read_only_stencil = false;
		bool disallow_merging = false;
		bool auto_preserve_all_other_attachments = true;
		MyVector<uint32_t> explicit_preserve_attachments = {};
		AttachmentReference2 fragment_shading_rate = {};
		Dyn<VkExtent2D> shading_rate_texel_size = {};
		Dyn<VkSampleCountFlagBits> inline_multisampling = {};
	};

	class RenderPassInstance : public AbstractInstance
	{
	protected:

		MyVector<VkAttachmentDescription2> _attachement_descriptors;
		MyVector<VkAttachmentReference2> _attachement_references;
		MyVector<uint32_t> _preserve_attachments_indices;
		MyVector<VkSubpassDescription2> _subpasses;
		MyVector<VkFragmentShadingRateAttachmentInfoKHR> _subpass_shading_rate;
		MyVector<VkMultisampledRenderToSingleSampledInfoEXT> _subpass_inline_multisampling;
		MyVector<VkSubpassDependency2> _dependencies;
		
		VkRenderPassCreationControlEXT _creation_control;
		VkRenderPassCreationFeedbackCreateInfoEXT _creation_feedback_ci;
		VkRenderPassCreationFeedbackInfoEXT _creation_feedback_info;
		struct SubpassCreationFeedback
		{
			VkRenderPassCreationControlEXT control;
			VkRenderPassSubpassFeedbackCreateInfoEXT ci;
			VkRenderPassSubpassFeedbackInfoEXT info;
		};
		MyVector<SubpassCreationFeedback> _subpass_creation_feedback;

		struct SubPassInfo
		{
			uint32_t shading_rate_index = uint32_t(-1);
			uint32_t inline_multisampling_index = uint32_t(-1);
			uint32_t creation_feedback_index = uint32_t(-1);
		};
		MyVector<SubPassInfo> _subpass_info;

		struct AttachmentSubpassUsage
		{
			using UsageUint = uint32_t;
			enum class Usage : UsageUint
			{
				None = 0x0,
				Input = 0x01,
				Color = 0x02,
				Depth = 0x04,
				Stencil = 0x08,
				DepthStencil = Depth | Stencil,
				ResolveDst = 0x10,
				ResolveSrc = 0x20,
				Preserve = 0x40,
				ShadingRate = 0x80,
			};
			Usage usage = Usage::None;
		};
		
		// of size # of attachments x # of subpasses
		MyVector<AttachmentSubpassUsage> _attachments_usages_per_subpass;
		uint32_t indexAttachmentUsagePerSubpass(uint32_t subpass, uint32_t attachment) const
		{
			return subpass * _attachement_descriptors.size32() + attachment;
		}

		struct AttachmentUsage
		{
			VkAccessFlags initial_access = VK_ACCESS_NONE;
			VkAccessFlags final_access = VK_ACCESS_NONE;
			VkPipelineStageFlags initial_stage = VK_PIPELINE_STAGE_NONE;
			VkPipelineStageFlags final_stage = VK_PIPELINE_STAGE_NONE;
			VkImageUsageFlags usage = 0;
			uint32_t first_subpass = uint32_t(-1);
			uint32_t last_subpass = uint32_t(-1);
			AttachmentDescription2::Flags flags = AttachmentDescription2::Flags::None;
		};
		MyVector<AttachmentUsage> _attachments_usages;

		VkRenderPassCreateInfo2 _vk_ci2;

		VkRenderPass _handle = VK_NULL_HANDLE;

		void setVulkanName();

		friend class RenderPass;

		struct CreateInfo;

		void construct(CreateInfo const& ci);

		void unpack(CreateInfo const& ci);

		void deduceUsages();

		void link();
		
		void prepareSynch();

	public:

		using __AttachmentSubpassUsage = AttachmentSubpassUsage;

		struct SubPassCreateInfo
		{
			VkSubpassDescription2 desc = {};
			VkFragmentShadingRateAttachmentInfoKHR shading_rate = {};
			VkMultisampledRenderToSingleSampledInfoEXT inline_multisampling = {};
			bool disallow_merging = false;
		};

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			bool create_vk_render_pass = false;
			bool query_creation_feedback = false;
			bool disallow_merging = false;
			MyVector<VkAttachmentDescription2> attachement_descriptors = {};
			MyVector<VkAttachmentReference2> attachement_references = {};
			MyVector<uint32_t> preserve_attachments_indices = {};
			MyVector<SubPassCreateInfo> subpasses = {};
		};
		using CI = CreateInfo;

		RenderPassInstance(CreateInfo const& ci);
		RenderPassInstance(CreateInfo && ci);

		virtual ~RenderPassInstance() override;



		void create(VkRenderPassCreateInfo2 const& ci);

		void destroy();

		constexpr operator VkRenderPass()const
		{
			return _handle;
		}

		constexpr VkRenderPass handle()const
		{
			return _handle;
		}

		constexpr const auto& getAttachmentDescriptors2()const
		{
			return _attachement_descriptors;
		}

		const auto& getAttachmentUsages() const
		{
			return _attachments_usages;
		}

		const auto& getSubpasses() const
		{
			return _subpasses;
		}

	};

	class RenderPass : public InstanceHolder<RenderPassInstance>
	{
	public:
		enum class Mode
		{
			Automatic = 0x0,
			RenderPass = 0x1,
			DynamicRendering = 0x2,
			//Both = RenderPass | DynamicRendering,
		};
	protected:

		using ParentType = InstanceHolder<RenderPassInstance>;

		Mode _mode;

		MyVector<AttachmentDescription2> _attachments;
		MyVector<SubPassDescription2> _subpasses;

		void createInstance();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Mode mode = Mode::Automatic;
			MyVector<AttachmentDescription2> attachments;
			MyVector<SubPassDescription2> subpasses;
			Dyn<bool> disallow_merging = {};
			bool create_on_construct = false;
			Dyn<bool> hold_instance = {};
		};
		using CI = CreateInfo;

		struct SinglePassCreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Mode mode = Mode::Automatic;
			uint32_t view_mask = 0;
			MyVector<AttachmentDescription2> colors = {};
			AttachmentDescription2 depth_stencil = {};
			MyVector<AttachmentDescription2> resolve = {};
			AttachmentDescription2 fragment_shading_rate = {};
			Dyn<VkExtent2D> fragment_shading_rate_texel_size = {};
			Dyn<VkSampleCountFlagBits> inline_multisampling = {};
			bool read_only_depth = false;
			bool read_only_stencil = false;
			bool create_on_construct = false;
			Dyn<bool> hold_instance = {};
		};
		using SPCI = SinglePassCreateInfo;

		RenderPass(CreateInfo const& ci);

		RenderPass(SinglePassCreateInfo const& spci);

		virtual ~RenderPass()override;

		bool updateResources(UpdateContext & ctx);

		constexpr const auto& attachments() const
		{
			return _attachments;
		}

		constexpr auto& subpasses() const
		{
			return _subpasses;
		}
	};

THAT_DECLARE_ENUM_CLASS_OPERATORS(vkl::AttachmentDescription2::Flags, vkl::AttachmentDescription2::FlagsUint)
THAT_DECLARE_ENUM_CLASS_OPERATORS(vkl::RenderPassInstance::__AttachmentSubpassUsage::Usage, vkl::RenderPassInstance::__AttachmentSubpassUsage::UsageUint)

}

