#pragma once

#include "VkApplication.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"

namespace vkl
{
    class DescriptorSet : public VkObject
    {
    protected:

        std::shared_ptr<DescriptorSetLayout> _layout = nullptr;
        std::shared_ptr<DescriptorPool> _pool = nullptr;

        VkDescriptorSet _handle = VK_NULL_HANDLE;

    public:

        constexpr DescriptorSet(VkApplication * app=nullptr) noexcept :
            VkObject(app)
        {}

        DescriptorSet(std::shared_ptr<DescriptorSetLayout> layout, std::shared_ptr<DescriptorPool> pool);
        
        DescriptorSet(std::shared_ptr<DescriptorSetLayout> layout, std::shared_ptr<DescriptorPool> pool, VkDescriptorSet handle);

        virtual ~DescriptorSet() override;

        void allocate(VkDescriptorSetAllocateInfo const& alloc);

        void destroy();

        constexpr operator VkDescriptorSet()const
        {
            return _handle;
        }

        constexpr VkDescriptorSet handle()const
        {
            return _handle;
        }

        constexpr const auto& layout()const
        {
            return _layout;
        }

        constexpr auto& layout()
        {
            return _layout;
        }

        constexpr const auto& pool()const
        {
            return _pool;
        }

        constexpr auto& pool()
        {
            return _pool;
        }

    };
} // namespace vkl
