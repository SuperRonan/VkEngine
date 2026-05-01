#pragma once

#include "AbstractInstance.hpp"
#include <cassert>
#include <utility>
#include <vkl/Execution/UpdateContext.hpp>

namespace vkl
{
	namespace GUI
	{
		class SamplerInstanceInspector;
		class SamplerInspector;
	}
	class SamplerInstance: public InstanceBase<VkSampler>
	{
	protected:
		using Parent = InstanceBase<VkSampler>;

		VkSamplerCreateInfo _ci = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkSamplerCreateInfo vk_ci = {};
		};
		using CI = CreateInfo;

		SamplerInstance(CreateInfo const& ci);

		virtual ~SamplerInstance() override;

		constexpr const VkSamplerCreateInfo& createInfo()const
		{
			return _ci;
		}

		using InspectorType = GUI::SamplerInstanceInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;

		static constexpr const char* ClassName = "Sampler";
	};

	class Sampler : public InstanceHolder<SamplerInstance>
	{
	public:
		using Parent = InstanceHolder<SamplerInstance>;

		constexpr static VkBorderColor defaultBorderColor()
		{
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}

		constexpr static VkSamplerMipmapMode mipMapModeFromFilter(VkFilter filter)
		{
			if (filter == VK_FILTER_NEAREST)	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			else //if (filter == VK_FILTER_LINEAR)
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

	protected:

		void createInstance();

		VkSamplerCreateInfo _vk_ci = {};

		virtual void updateResourcesInline(UpdateContext& ctx, UpdateResourcesResult& res) override;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			VkSamplerCreateFlags flags = 0;
			VkFilter filter = VK_FILTER_NEAREST;
			VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			std::optional<float> max_anisotropy = {};
			VkBorderColor border_color = {};
			VkBool32 unnormalized_coordinates = false;
			std::optional<VkCompareOp> compare_op = {};
			bool create_on_construct = false;
			Dyn<bool> hold_instance = true;
		};
		using CI = CreateInfo;

		Sampler(CreateInfo const& ci);

		Sampler(std::shared_ptr<SamplerInstance> const& inst);

		virtual ~Sampler() override;

		static std::shared_ptr<Sampler> MakeNearest(VkApplication * app = nullptr);

		static std::shared_ptr<Sampler> MakeBilinear(VkApplication* app = nullptr);

		using InspectorType = GUI::SamplerInspector;
		friend class InspectorType;
		virtual std::shared_ptr<GUI::Panel> makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx) override;
	};

	VKL_DEFINE_DESCRIPTOR_INSTANCE_POINTERS(Sampler)
}