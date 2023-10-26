#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ExecutionContext.hpp>
#include <Core/Execution/SynchronizationHelper.hpp>
#include <Core/VkObjects/Pipeline.hpp>

namespace vkl
{
	struct VertexBuffer
	{
		std::shared_ptr<Buffer> buffer = nullptr;
		Range_st range = {};
	};
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

			struct VertexDrawCallResources
			{
				uint32_t draw_count;
				uint32_t instance_count;
				std::shared_ptr<Buffer> index_buffer = nullptr;
				Range_st index_buffer_range;
				VkIndexType index_type;
				std::vector<VertexBuffer> vertex_buffers;
			};

			virtual void fillVertexDrawCallResources(VertexDrawCallResources & vr) = 0;
	};
}