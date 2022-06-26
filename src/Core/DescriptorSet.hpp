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

        VkDescriptorSet _handle = VK_NULL_HANDLE;

    public:


    };
} // namespace vkl
