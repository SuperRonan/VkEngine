#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ExecutionContext.hpp>
#include <Core/Execution/SynchronizationHelper.hpp>
#include <Core/VkObjects/Pipeline.hpp>

namespace vkl
{
	class Drawable
	{
		public:

			Drawable()
			{}

			virtual void recordSynchForDraw(SynchronizationHelper & synch, std::shared_ptr<Pipeline> const& pipeline) = 0;

			virtual VertexInputDescription vertexInputDesc() = 0;

			virtual void recordBindAndDraw(ExecutionContext & ctx) = 0;

			virtual std::shared_ptr<DescriptorSetLayout> setLayout() = 0;

			virtual std::shared_ptr<DescriptorSetAndPool> setAndPool() = 0;
	};
}