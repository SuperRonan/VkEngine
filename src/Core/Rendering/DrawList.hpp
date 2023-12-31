#pragma once

#include <Core/VulkanCommons.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>

#include <cassert>


namespace vkl
{
	class VertexDrawList
	{
	public:
		struct DrawCallInfo
		{
			std::string name = {};

			uint32_t draw_count = 0;
			uint32_t instance_count = 0;
			BufferAndRange index_buffer = {};
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			uint32_t vertex_buffer_begin = 0;
			uint32_t num_vertex_buffers = 0;

			std::shared_ptr<DescriptorSetAndPool> set = nullptr;
			PushConstant pc = {};
		};
	protected:

		// Maybe "cache" the names in a separate buffer like the vertex buffers
		// But since std::string already has a small buffer optimisation, not necessary
		
		Array<BufferAndRange> _vertex_buffers = {};
		Array<DrawCallInfo> _draw_calls = {};

	public:

		void clear();

		BufferAndRange* push_back(DrawCallInfo const& dci);

		template <std::concepts::ContainerMaybeRef<BufferAndRange> VBs>
		void push_back(DrawCallInfo const& dci, VBs&& vbs)
		{
			BufferAndRange* ptr = push_back(dci);
			assert(vbs.size() == dci.num_vertex_buffers);
			std::copy(vbs.begin(), vbs.end(), ptr);
		}

		size_t size() const
		{
			return _draw_calls.size();
		}

		uint32_t size32()const
		{
			return size();
		}

		const Array<DrawCallInfo>& drawCalls() const
		{
			return _draw_calls;
		}

		const Array<BufferAndRange>& vertexBuffers() const
		{
			return _vertex_buffers;
		}

	};
}