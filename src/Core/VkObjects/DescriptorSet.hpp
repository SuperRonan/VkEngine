#pragma once

#include <Core/App/VkApplication.hpp>
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

        void setVkName();

        void allocate(VkDescriptorSetAllocateInfo const& alloc);

        void destroy();

    public:

        struct CreateInfo
        {
            VkApplication * app = nullptr;
            std::string name = {};
            std::shared_ptr<DescriptorSetLayout> layout = nullptr;
            std::shared_ptr<DescriptorPool> pool = nullptr;
        };
        using CI = CreateInfo;

        DescriptorSet(CreateInfo const& ci);
        
        virtual ~DescriptorSet() override;


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
