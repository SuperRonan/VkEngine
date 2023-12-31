#include "DrawList.hpp"

namespace vkl
{
	void VertexDrawList::clear()
	{
		_vertex_buffers.clear();
		_draw_calls.clear();
	}

	BufferAndRange* VertexDrawList::push_back(DrawCallInfo const& dci)
	{
		BufferAndRange* res = nullptr;

		_draw_calls.push_back(dci);

		if (_draw_calls.back().num_vertex_buffers > 0)
		{
			_draw_calls.back().vertex_buffer_begin = _vertex_buffers.size32();
			_vertex_buffers.resize(_vertex_buffers.size() + _draw_calls.back().num_vertex_buffers);
			res = _vertex_buffers.data() + _draw_calls.back().vertex_buffer_begin;
		}

		return res;
	}
}