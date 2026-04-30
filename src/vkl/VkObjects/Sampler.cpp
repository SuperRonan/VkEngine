#include <vkl/VkObjects/Sampler.hpp>

#include <vkl/GUI/DescriptorInstancePanel.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>

namespace vkl
{
	SamplerInstance::SamplerInstance(CreateInfo const& ci) :
		Parent(ci.app, ci.name),
		_ci(ci.vk_ci)
	{
		VK_CHECK(vkCreateSampler(device(), &_ci, nullptr, &handle()), "Failed to create a sampler.");
		registerName();
	}

	SamplerInstance::~SamplerInstance()
	{
		assert(handle() != VK_NULL_HANDLE);
		callDestructionCallbacks();
		vkDestroySampler(device(), handle(), nullptr);
		handle() = VK_NULL_HANDLE;
	}

	Sampler::Sampler(CreateInfo const& ci) :
		InstanceHolder<SamplerInstance>(ci.app, ci.name, ci.hold_instance)
	{
		_vk_ci = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = ci.filter,
			.minFilter = ci.filter,
			.mipmapMode = mipMapModeFromFilter(ci.filter),
			.addressModeU = ci.address_mode,
			.addressModeV = ci.address_mode,
			.addressModeW = ci.address_mode,
			.mipLodBias = 0,
			.anisotropyEnable = ci.max_anisotropy.has_value(),
			.maxAnisotropy = ci.max_anisotropy.value_or(0.0f), 
			.compareEnable = ci.compare_op.has_value() ? VK_TRUE : VK_FALSE,
			.compareOp = ci.compare_op.value_or(VK_COMPARE_OP_NEVER),
			.minLod = 0,
			.maxLod = 16, // TODO
			.borderColor = ci.border_color,
			.unnormalizedCoordinates = ci.unnormalized_coordinates,
		};

		if (ci.create_on_construct)
		{
			createInstance();
		}
	}

	Sampler::~Sampler()
	{
		
	}

	void Sampler::createInstance()
	{
		assert(!_instance);
		
		_instance = std::make_shared<SamplerInstance>(SamplerInstance::CI{
			.app = application(),
			.name = name(),
			.vk_ci = _vk_ci,
		});
	}

	void Sampler::updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res)
	{
		if (!_instance)
		{
			res.created = true;
			createInstance();
		}
	}

	std::shared_ptr<Sampler> Sampler::MakeNearest(VkApplication* app)
	{
		return std::make_shared<Sampler>(Sampler::CI{
			.app = app,
			.name = "NearestSampler",
			.filter = VK_FILTER_NEAREST,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});
	}

	std::shared_ptr<Sampler> Sampler::MakeBilinear(VkApplication* app)
	{
		return std::make_shared<Sampler>(Sampler::CI{
			.app = app,
			.name = "LinearSampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		});
	}

	namespace GUI
	{
		class SamplerInstanceInspector : public InstanceInspector<SamplerInstance>
		{
		protected:

		public:

			SamplerInstanceInspector(std::shared_ptr<SamplerInstance> const& target):
				InstanceInspector<SamplerInstance>(target)
			{

			}

			virtual void declareInline(Context& ctx)
			{
				ImGui::LabelPointer("Handle", target()->handle());
				VkSamplerCreateInfo const& ci = target()->createInfo();
				GUI::InspectVkBitField<VkSamplerCreateFlagBits>(ctx, "Flags", ci.flags);
				GUI::InspectVkEnum(ctx, "Mag Filter", ci.magFilter);
				GUI::InspectVkEnum(ctx, "Min Filter", ci.minFilter);
				GUI::InspectVkEnum(ctx, "MipMap Mode", ci.mipmapMode);
				GUI::InspectVkEnum(ctx, "Address Mode U", ci.addressModeU);
				GUI::InspectVkEnum(ctx, "Address Mode V", ci.addressModeV);
				GUI::InspectVkEnum(ctx, "Address Mode W", ci.addressModeW);
				float mip_lod_bias = ci.mipLodBias;
				ImGui::InputFloat("Mip LOD Bias", &mip_lod_bias, 0, 0, nullptr, ImGuiInputTextFlags_ReadOnly);
				bool anisotropy = ci.anisotropyEnable != VK_FALSE;
				ImGui::LabelCheckbox("Anisotropy", anisotropy);
				ImGui::BeginDisabled(!anisotropy);
				{
					float max_anisotropy = ci.maxAnisotropy;
					ImGui::InputFloat("Max Anisotropy", &max_anisotropy, 0, 0, nullptr, ImGuiInputTextFlags_ReadOnly);
				}
				ImGui::EndDisabled();
				bool compare = ci.compareEnable != VK_FALSE;
				ImGui::LabelCheckbox("Compare", compare);
				ImGui::BeginDisabled(!compare);
				{
					GUI::InspectVkEnum(ctx, "Compare Op", ci.compareOp);
				}
				ImGui::EndDisabled();
				float lod_range[2] = {ci.minLod, ci.maxLod};
				ImGui::InputFloat2("LOD Range", lod_range, nullptr, ImGuiInputTextFlags_ReadOnly);
				GUI::InspectVkEnum(ctx, "Border Color Mode", ci.borderColor);
				ImGui::LabelCheckbox("UnNormalized Coordinates", ci.unnormalizedCoordinates != VK_FALSE);
			}
		};

		class SamplerInspector : public DescriptorInspector<Sampler>
		{
		protected:

		public:

			SamplerInspector(std::shared_ptr<Sampler> const& target):
				DescriptorInspector<Sampler>(target)
			{

			}

			virtual void declareInline(Context& ctx) override
			{
				
				ImGui::Separator();
				declareInstance(ctx);
			}
		};
	}

	std::shared_ptr<GUI::Panel> SamplerInstance::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return MakeInspectorFromTarget(ctx, std::static_pointer_cast<SamplerInstance>(shared_this));
	}

	std::shared_ptr<GUI::Panel> Sampler::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return MakeInspectorFromTarget(ctx, std::static_pointer_cast<Sampler>(shared_this));
	}
}