#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>

namespace vkl
{
	struct VertexDrawCallInfo
	{
		std::string name = {};

		uint32_t draw_count = 0;
		uint32_t instance_count = 0;
		BufferAndRange index_buffer = {};
		VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
		Array<BufferAndRange> vertex_buffers = {};

		std::shared_ptr<DescriptorSetAndPool> set = nullptr;
		PushConstant pc = {};

		void clear()
		{
			name.clear();
			draw_count = 0;
			instance_count = 0;
			index_buffer = {};
			index_type = VK_INDEX_TYPE_MAX_ENUM;
			vertex_buffers.clear();
			set.reset();
			pc.clear();
		}
	};

	class Drawable
	{
	public:

		Drawable()
		{}

		virtual VertexInputDescription vertexInputDesc() = 0;

		virtual void fillVertexDrawCallInfo(VertexDrawCallInfo& vdl) = 0;
	};
}