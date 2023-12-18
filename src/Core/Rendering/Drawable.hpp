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

			virtual VertexInputDescription vertexInputDesc() = 0;

			//virtual void recordSynchForDraw(SynchronizationHelper & synch, std::shared_ptr<Pipeline> const& pipeline) = 0;

			//virtual void recordBindAndDraw(ExecutionContext & ctx) = 0;

			//virtual std::shared_ptr<DescriptorSetLayout> setLayout() = 0;

			//virtual std::shared_ptr<DescriptorSetAndPool> setAndPool() = 0;

			struct VertexDrawCallInfo
			{
				uint32_t draw_count;
				uint32_t instance_count;
				BufferAndRange index_buffer;
				VkIndexType index_type;
				Array<BufferAndRange> vertex_buffers;
			};

			virtual void fillVertexDrawCallInfo(VertexDrawCallInfo & vr) = 0;
	};
}