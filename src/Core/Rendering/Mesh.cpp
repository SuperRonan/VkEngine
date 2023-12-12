#include "Mesh.hpp"
#include <numeric>
#include <numbers>
#include <Core/VkObjects/Pipeline.hpp>
#include <Core/Execution/SynchronizationHelper.hpp>

namespace vkl
{
	
	Mesh::Mesh(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		_type(ci.type),
		_is_synch(ci.synch)
	{}


	VertexInputDescription RigidMesh::vertexInputDescFullVertex()
	{
		VertexInputDescription res;
		res.binding = {
			VkVertexInputBindingDescription{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};

		res.attrib = {
			VkVertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, position),
			},
			VkVertexInputAttributeDescription{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, normal),
			},
			VkVertexInputAttributeDescription{
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, tangent),
			},
			VkVertexInputAttributeDescription{
				.location = 3,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, uv),
			},
		};

		return res;
	}

	VertexInputDescription RigidMesh::vertexInputDescOnlyPos3D()
	{
		VertexInputDescription res;
		res.binding = {
			VkVertexInputBindingDescription{
				.binding = 0,
				.stride = sizeof(vec3),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};

		res.attrib = {
			VkVertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = 0,
			},
		};

		return res;
	}

	VertexInputDescription RigidMesh::vertexInputDescOnlyPos2D()
	{
		VertexInputDescription res;
		res.binding = {
			VkVertexInputBindingDescription{
				.binding = 0,
				.stride = sizeof(vec2),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};

		res.attrib = {
			VkVertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = 0,
			},
		};

		return res;
	}

	bool RigidMesh::checkIntegrity() const
	{
		bool res = true;
		for (size_t i=0; i<_host.indicesSize(); ++i)
		{
			res &= (_host.getIndex(i) < _host.vertices.size());
			assert(res);
		}
		return res;
	}

	RigidMesh::RigidMesh(CreateInfo const& ci) :
		Mesh(Mesh::CreateInfo{
			.app = ci.app, 
			.name = ci.name,
			.type = Type::Rigid,
			.synch = ci.synch,
		})
	{
		_host.dims = ci.dims;
		_host.positions = ci.positions;
		_host.vertices = ci.vertices;
		_host.indices32 = ci.indices;
		if (_host.vertices.empty())
		{
			_host.use_full_vertices = false;
		}
		_host.index_type = VK_INDEX_TYPE_UINT32;
		_host.loaded = true;

		compressIndices();

		if(_host.use_full_vertices)
		{
			if (ci.compute_normals != 0)
			{
				computeNormals(ci.compute_normals);
			}

			if (ci.auto_compute_tangents)
			{
				computeTangents();
			}
		}

		computeAABB();

		if (ci.create_device_buffer)
		{
			createDeviceBuffer({});
		}
	}
	

	RigidMesh::~RigidMesh()
	{
		
	}

	MeshHeader RigidMesh::getHeader() const
	{
		return MeshHeader{
			.num_vertices = static_cast<uint32_t>(_host.numVertices()),
			.num_indices = static_cast<uint32_t>(_host.indicesSize()),
			.num_primitives = static_cast<uint32_t>(_host.indicesSize() / 3),
			.flags = meshFlags(_host.index_type),
		};
	}

	void RigidMesh::compressIndices()
	{
		const bool index_uint8_t_available = application()->availableFeatures().index_uint8_ext.indexTypeUint8;
		const size_t vs = _host.numVertices();
		const size_t is = _host.indicesSize();
		const bool can8 = vs <= UINT8_MAX && index_uint8_t_available;
		const bool can16 = vs <= UINT16_MAX;
		const bool can32 = vs <= UINT32_MAX;
		VkIndexType & idxt = _host.index_type;
		if (can8)
		{
			if (idxt == VK_INDEX_TYPE_UINT16)
			{
				std::vector<u16> tmp = std::move(_host.indices16);
				_host.indices8 = std::vector<u8>(is);
				std::transform(tmp.begin(), tmp.end(), _host.indices8.begin(), [](u16 i){return static_cast<u8>(i);});
				//std::copy(tmp.begin(), tmp.end(), _host.indices8.begin());
				_device.up_to_date = false;
			}
			else if (idxt == VK_INDEX_TYPE_UINT32)
			{
				std::vector<u32> tmp = std::move(_host.indices32);
				_host.indices8 = std::vector<u8>(is);
				std::transform(tmp.begin(), tmp.end(), _host.indices8.begin(), [](u32 i) {return static_cast<u8>(i); });
				//std::copy(tmp.begin(), tmp.end(), _host.indices8.begin());
				_device.up_to_date = false;
			}
			else
			{
				assert(idxt == VK_INDEX_TYPE_UINT8_EXT);
			}
			idxt = VK_INDEX_TYPE_UINT8_EXT;
		}
		else if (can16)
		{
			if (idxt == VK_INDEX_TYPE_UINT32)
			{
				std::vector<u32> tmp = std::move(_host.indices32);
				_host.indices16 = std::vector<u16>(is);
				std::transform(tmp.begin(), tmp.end(), _host.indices16.begin(), [](u32 i) {return static_cast<u16>(i); });
				//std::copy(tmp.begin(), tmp.end(), _host.indices16.begin());
				_device.up_to_date = false;
			}
			else
			{
				assert(idxt == VK_INDEX_TYPE_UINT16);
			}
			idxt = VK_INDEX_TYPE_UINT16;
		}
		else if (can32)
		{
			assert(idxt == VK_INDEX_TYPE_UINT32);
		}
		else
		{
			// Should not be here
			assert(false);
		}
	}

	void RigidMesh::decompressIndices()
	{
		if (_host.index_type == VK_INDEX_TYPE_UINT8_EXT)
		{
			std::vector<u8> tmp = std::move(_host.indices8);
			_host.indices32 = std::vector<u32>(tmp.size());
			std::copy(tmp.begin(), tmp.end(), _host.indices32.begin());
			_device.up_to_date = false;
		}
		else if (_host.index_type == VK_INDEX_TYPE_UINT16)
		{
			std::vector<u16> tmp = std::move(_host.indices16);
			_host.indices32 = std::vector<u32>(tmp.size());
			std::copy(tmp.begin(), tmp.end(), _host.indices32.begin());
			_device.up_to_date = false;
		}
		else
		{
			assert(_host.index_type == VK_INDEX_TYPE_UINT32);
		}
		_host.index_type = VK_INDEX_TYPE_UINT32;
	}

	void RigidMesh::merge(RigidMesh const& other) noexcept
	{
		assert(_host.use_full_vertices == other._host.use_full_vertices);
		assert(_host.dims == other._host.dims);
		const size_t offset = _host.numVertices();
		if (_host.use_full_vertices)
		{
			_host.vertices.resize(offset + other._host.vertices.size());
			std::copy(other._host.vertices.begin(), other._host.vertices.end(), _host.vertices.begin() + offset);
		}
		else
		{
			_host.positions.resize(_host.positions.size() + other._host.positions.size());
			std::copy(other._host.positions.begin(), other._host.positions.end(), _host.positions.begin() + (offset * _host.dims));
		}
		
		const size_t old_size = _host.indicesSize();
		const size_t new_size = old_size + other._host.indicesSize();
		decompressIndices();
		{
			_host.indices32.resize(new_size);
			switch (other._host.index_type)
			{
			case VK_INDEX_TYPE_UINT32:
				for(size_t i = 0; i < other._host.indices32.size(); ++i)
					_host.indices32[i + old_size] = other._host.indices32[i] + offset;
			break;
			case VK_INDEX_TYPE_UINT16:
				for (size_t i = 0; i < other._host.indices16.size(); ++i)
					_host.indices32[i + old_size] = other._host.indices16[i] + offset;
			break;
			case VK_INDEX_TYPE_UINT8_EXT:
				for (size_t i = 0; i < other._host.indices8.size(); ++i)
					_host.indices32[i + old_size] = other._host.indices8[i] + offset;
			break;
			}
		}
		compressIndices();
	}

	RigidMesh& RigidMesh::operator+=(RigidMesh const& other) noexcept
	{
		merge(other);
		return *this;
	}

	void RigidMesh::transform(Matrix4 const& m)
	{
		const Matrix3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
		for (Vertex& vertex : _host.vertices)
		{
			vertex.transform(m, nm);
		}
	}

	RigidMesh& RigidMesh::operator*=(Matrix4 const& m)
	{
		transform(m);
		return *this;
	}

	void RigidMesh::computeTangents()
	{
		using Float = float;
		// https://marti.works/posts/post-calculating-tangents-for-your-mesh/post/
		// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
		uint number_of_triangles = _host.indicesSize() / 3;

		struct Tan
		{
			alignas(16) Vector3 tg = Vector3(0);
			alignas(16) Vector3 btg = Vector3(0);
		};

		
		std::vector<Tan> tans(_host.vertices.size());

		for (uint triangle_index = 0; triangle_index < number_of_triangles; ++triangle_index)
		{
			uint i0 = _host.getIndex(triangle_index * 3);
			uint i1 = _host.getIndex(triangle_index * 3 + 1);
			uint i2 = _host.getIndex(triangle_index * 3 + 2);

			Vector3 pos0 = _host.vertices[i0].position;
			Vector3 pos1 = _host.vertices[i1].position;
			Vector3 pos2 = _host.vertices[i2].position;

			Vector2 tex0 = _host.vertices[i0].uv;
			Vector2 tex1 = _host.vertices[i1].uv;
			Vector2 tex2 = _host.vertices[i2].uv;

			Vector3 edge1 = pos1 - pos0;
			Vector3 edge2 = pos2 - pos0;

			Vector2 uv1 = tex1 - tex0;
			Vector2 uv2 = tex2 - tex0;

			// Implicit matrix [uv1[1], -uv1[0]; -uv2[1], uv2[0]]
			// The determinent of this matrix
			Float d = Float(1) / (uv1[0] * uv2[1] - uv1[1] * uv2[0]);

			Vector3 tg = Vector3{
				((edge1[0] * uv2[1]) - (edge2[0] * uv1[1])),
				((edge1[1] * uv2[1]) - (edge2[1] * uv1[1])),
				((edge1[2] * uv2[1]) - (edge2[2] * uv1[1])),
			} *d;

			Vector3 btg = Vector3{
				((edge1[0] * uv2[0]) - (edge2[0] * uv1[0])),
				((edge1[1] * uv2[0]) - (edge2[1] * uv1[0])),
				((edge1[2] * uv2[0]) - (edge2[2] * uv1[0])),
			} *d;

			tans[i0].tg += tg;
			tans[i1].tg += tg;
			tans[i2].tg += tg;

			tans[i0].btg += btg;
			tans[i1].btg += btg;
			tans[i2].btg += btg;
		}

		for (uint vertex_id = 0; vertex_id < _host.vertices.size(); ++vertex_id)
		{
			Vector3 normal = _host.vertices[vertex_id].normal;
			Vector3 tg = tans[vertex_id].tg;
			Vector3 btg = tans[vertex_id].btg;

			Vector3 t = tg - (normal * glm::dot(normal, tg));
			t = glm::normalize(t);

			Vector3 c = glm::cross(normal, tg);
			Float w = (glm::dot(c, btg) < 0) ? -1 : 1;
			_host.vertices[vertex_id].tangent = t * w;
		}
	}

	void RigidMesh::computeNormals(int mode)
	{
		const size_t N = _host.indicesSize();

		for (Vertex& v : _host.vertices)
		{
			v.normal = vec3(0);
		}

		for (size_t i = 0; i < N; i += 3)
		{
			const size_t i0 = _host.getIndex(i + 0);
			const size_t i1 = _host.getIndex(i + 1);
			const size_t i2 = _host.getIndex(i + 2);

			Vertex& v0 = _host.vertices[i0];
			Vertex& v1 = _host.vertices[i1];
			Vertex& v2 = _host.vertices[i2];

			vec3 e1 = v1.position - v0.position;
			vec3 e2 = v2.position - v0.position;

			vec3 e1n = glm::normalize(e1);
			vec3 e2n = glm::normalize(e2);

			vec3 face_normal = glm::normalize(glm::cross(e1n, e2n));

			// Use some weighting? 
			// - based on triangle surface?
			// - based on vertex angle? 

			v0.normal += face_normal;
			v1.normal += face_normal;
			v2.normal += face_normal;
		}

		for (Vertex& v : _host.vertices)
		{
			if (glm::dot(v.normal, v.normal) != 0)
			{
				v.normal = glm::normalize(v.normal);
			}
		}

		_device.up_to_date = false;
	}

	void RigidMesh::computeAABB()
	{
		_aabb.clear();
		if (_host.use_full_vertices)
		{
			for (size_t i = 0; i < _host.vertices.size(); ++i)
			{
				_aabb += _host.vertices[i].position;
			}
		}
		else
		{	
			size_t N = _host.numVertices();
			for (size_t i = 0; i < (N * _host.dims); i += _host.dims)
			{
				vec3 p = vec3(0);
				for (uint8_t d = 0; d < _host.dims; ++d)
				{
					p[d] = _host.positions[i + d];
				}
				_aabb += p;
			}
		}
	}

	void RigidMesh::flipFaces()
	{
		const size_t N = _host.indicesSize() / 3;
		for (size_t t = 0; t < N; ++t)
		{
			switch (_host.index_type)
			{
			case VK_INDEX_TYPE_UINT32:
				std::swap(_host.indices32[3 * t + 0], _host.indices32[3 * t + 2]);
				break;
			case VK_INDEX_TYPE_UINT16:
				std::swap(_host.indices16[3 * t + 0], _host.indices16[3 * t + 2]);
				break;
			case VK_INDEX_TYPE_UINT8_EXT:
				std::swap(_host.indices8[3 * t + 0], _host.indices8[3 * t + 2]);
				break;
			}
		}
	}

	void RigidMesh::createDeviceBuffer(std::vector<uint32_t> const& queues)
	{
		assert(_host.loaded);
		assert(!_device.loaded());

		const MeshHeader header = getHeader();

		const size_t ssbo_align = application()->deviceProperties().props.limits.minStorageBufferOffsetAlignment;
		const size_t ubo_align = application()->deviceProperties().props.limits.minUniformBufferOffsetAlignment;
		
		_device.header_size = std::align(sizeof(header), ssbo_align);
		if (_host.use_full_vertices)
		{
			_device.vertices_size = std::align(header.num_vertices * sizeof(Vertex), ssbo_align);
		}
		else
		{
			_device.vertices_size = std::align(header.num_vertices * sizeof(float) * _host.dims, ssbo_align);
		}
		_device.indices_size = std::align(_host.indexBufferSize(), ssbo_align);
		_device.total_buffer_size = _device.header_size + _device.vertices_size + _device.indices_size;

		_device.num_indices = header.num_indices;
		_device.num_vertices = header.num_vertices;
		_device.index_type = _host.index_type;
		
		_device.mesh_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = _app,
			.name = name() + ".mesh_buffer",
			.size = &_device.total_buffer_size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_BITS | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.queues = queues,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
	}

	//void RigidMesh::recordBindAndDraw(ExecutionContext & ctx)
	//{
	//	VkCommandBuffer command_buffer = ctx.getCommandBuffer()->handle();
	//	assert(_device.loaded());
	//	VkBuffer vb = *_device.mesh_buffer->instance();
	//	VkDeviceSize offset = _device.header_size;
	//	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vb, &offset);
	//	vkCmdBindIndexBuffer(command_buffer, *_device.mesh_buffer->instance(), _device.vertices_size + _device.header_size, _device.index_type);
	//	vkCmdDrawIndexed(command_buffer, _device.num_indices, 1, 0, 0, 0);
	//}

	void RigidMesh::fillVertexDrawCallResources(VertexDrawCallResources& vr)
	{
		assert(_device.loaded());
		vr.draw_count = _device.num_indices;
		vr.instance_count = 1;
		vr.index_buffer = _device.mesh_buffer;
		const size_t index_size = [&]() {
			size_t res = 0;
			switch (_device.index_type)
			{
				case VK_INDEX_TYPE_UINT16:
					res = 2;
				break;
				case VK_INDEX_TYPE_UINT32:
					res = 4;
				break;
				case VK_INDEX_TYPE_UINT8_EXT:
					res = 1;
				break;
			}
			return res;
		}();
		vr.index_buffer_range = {.begin = _device.vertices_size + _device.header_size, .len = _device.num_indices * index_size};
		vr.index_type = _device.index_type;
		size_t vertex_size = _host.use_full_vertices ? sizeof(Vertex) : (_host.dims * sizeof(float));
		vr.vertex_buffers = {
			VertexBuffer{
				.buffer = _device.mesh_buffer,
				.range = {.begin = _device.header_size, .len = _device.num_vertices * vertex_size},
			},
		};
	}

	Mesh::Status RigidMesh::getStatus() const
	{
		return Status{
			.host_loaded = _host.loaded,
			.device_loaded = _device.loaded(),
			.device_up_to_date = _device.up_to_date,
		};
	}

	std::vector<DescriptorSetLayout::Binding> RigidMesh::getSetLayoutBindingsStatic(uint32_t offset)
	{
		std::vector<DescriptorSetLayout::Binding> res;
		using namespace std::containers_operators;

		res += {
			.vk_binding = VkDescriptorSetLayoutBinding{
				.binding = offset + 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			},
			.meta = DescriptorSetLayout::BindingMeta{
				.name = "MeshHeader",
				.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			},
		};

		res += {
			.vk_binding = VkDescriptorSetLayoutBinding{
				.binding = offset + 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			},
			.meta = DescriptorSetLayout::BindingMeta{
				.name = "MeshVertices",
				.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			}
		};

		res += {
			.vk_binding = VkDescriptorSetLayoutBinding{
				.binding = offset + 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			},
			.meta = DescriptorSetLayout::BindingMeta{
				.name = "MeshIndices32",
				.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			},
		};

		return res;
	}

	ShaderBindings RigidMesh::getShaderBindings(uint32_t offset)
	{
		assert(!!_device.mesh_buffer);
		using namespace std::containers_operators;
		ShaderBindings res;
		res += ShaderBindingDescription{
			.buffer = _device.mesh_buffer,
			.buffer_range = Range_st{.begin = 0, .len = _device.header_size},
			.binding = offset + 0,
		};

		res += ShaderBindingDescription{
			.buffer = _device.mesh_buffer,
			.buffer_range = Range_st{ .begin = _device.header_size, .len = _device.vertices_size },
			.binding = offset + 1,
		};

		res += ShaderBindingDescription{
			.buffer = _device.mesh_buffer,
			.buffer_range = Range_st{ .begin = _device.header_size + _device.vertices_size, .len = _device.indices_size },
			.binding = offset + 2,
		};
		return res;
	}

	void RigidMesh::updateResources(UpdateContext& ctx)
	{
		if (_device.mesh_buffer)
		{
			_device.mesh_buffer->updateResource(ctx);

			if (!_device.up_to_date)
			{
				_device.up_to_date = true;
				bool synch_upload = _is_synch;
				if (!_is_synch && ctx.uploadQueue() == nullptr)
				{
					synch_upload = true;
				}

				std::vector<PositionedObjectView> sources(3);
				const MeshHeader header = getHeader();
				sources[0] = PositionedObjectView{
					.obj = header,
					.pos = 0,
				};

				if (_host.use_full_vertices)
				{
					sources[1] = PositionedObjectView{
						.obj = _host.vertices,
						.pos = _device.header_size,
					};
				}
				else
				{
					sources[1] = PositionedObjectView{
						.obj = _host.positions,
						.pos = _device.header_size,
					};
				}

				sources[2] = PositionedObjectView{
					.obj = _host.indicesView(),
					.pos = _device.header_size + _device.vertices_size,
				};

				if (synch_upload)
				{
					ctx.resourcesToUpload() += ResourcesToUpload::BufferUpload{
						.sources = std::move(sources),
						.dst = _device.mesh_buffer->instance(),
					};
					_device.uploaded = true;
					callResourceUpdateCallbacks();
				}
				else
				{
					_device.uploaded = false;
					_device.just_uploaded = false;
					ctx.uploadQueue()->enqueue(AsynchUpload{
						.name = name(),
						.sources = std::move(sources),
						.target_buffer = _device.mesh_buffer->instance(),
						.completion_callback = [this](int) {
							_device.just_uploaded = true;
						},
					});
				}
			}
			else
			{
				if ( _device.just_uploaded)
				{
					_device.uploaded = true;
					_device.just_uploaded = false;
					callResourceUpdateCallbacks();
				}
			}
		}
	}

	void RigidMesh::installResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t offset)
	{
		_descriptor_callbacks.push_back(Callback{
			.callback = [this, set, offset]() {
				set->setBinding(Binding{
					.buffer = _device.mesh_buffer,
					.buffer_range = Range_st{.begin = 0, .len = _device.header_size},
					.binding = offset + 0,
				});
				set->setBinding(Binding{
					.buffer = _device.mesh_buffer,
					.buffer_range = Range_st{.begin = _device.header_size, .len = _device.vertices_size },
					.binding = offset + 1,
				});
				set->setBinding(Binding{
					.buffer = _device.mesh_buffer,
					.buffer_range = Range_st{.begin = _device.header_size + _device.vertices_size, .len = _device.indices_size },
					.binding = offset + 2,
				});
			},
			.id = set.get(),
		});
	}

	void RigidMesh::removeResourceUpdateCallbacks(std::shared_ptr<DescriptorSetAndPool> const& set)
	{
		for (size_t i = 0; i < _descriptor_callbacks.size(); ++i)
		{
			if (_descriptor_callbacks[i].id == set.get())
			{
				_descriptor_callbacks.erase(_descriptor_callbacks.begin() + i);
				break;
			}
		}
	}

	void RigidMesh::callResourceUpdateCallbacks()
	{
		for (size_t i = 0; i < _descriptor_callbacks.size(); ++i)
		{
			_descriptor_callbacks[i].callback();
		}
	}

	bool RigidMesh::isReadyToDraw()const
	{
		return _device.uploaded;
	}

	void RigidMesh::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());

		ImGui::Text("Name: ");
		ImGui::SameLine();
		ImGui::Text(name().c_str());

		ImGui::PopID();
	}

	//void RigidMesh::recordSynchForDraw(SynchronizationHelper& synch, std::shared_ptr<Pipeline> const& pipeline)
	//{
	//	//const bool separate_resource = false;
	//	//if (separate_resource)
	//	//{
	//	//	return Resources{
	//	//		Resource{
	//	//			._buffer = _device.mesh_buffer,
	//	//			._buffer_range = Range{.begin = _device.header_size, .len = _device.vertices_size},
	//	//			._begin_state = ResourceState2{
	//	//				.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
	//	//				.stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
	//	//			},
	//	//			._buffer_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	//	//		},
	//	//		Resource{
	//	//			._buffer = _device.mesh_buffer,
	//	//			._buffer_range = Range{.begin = _device.header_size + _device.vertices_size, .len = _device.indices_size},
	//	//			._begin_state = ResourceState2{
	//	//				.access = VK_ACCESS_2_INDEX_READ_BIT,
	//	//				.stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
	//	//			},
	//	//			._buffer_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	//	//		},
	//	//	};
	//	//}
	//	//else
	//	//{
	//	//	return Resources{
	//	//		Resource{
	//	//			._buffer = _device.mesh_buffer,
	//	//			._buffer_range = Range{.begin = _device.header_size, .len = _device.vertices_size + _device.indices_size},
	//	//			._begin_state = ResourceState2{
	//	//				.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT,
	//	//				.stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
	//	//			},
	//	//			._buffer_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	//	//		},
	//	//	};
	//	//}
	//
	//	synch.addSynch(Resource{
	//		._buffer = _device.mesh_buffer,
	//		._buffer_range = Range_st{.begin = _device.header_size, .len = _device.vertices_size + _device.indices_size},
	//		._begin_state = ResourceState2{
	//			.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT,
	//			.stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
	//		},
	//	});
	//}1

	std::shared_ptr<RigidMesh> RigidMesh::MakeSquare(Square2DMakeInfo const& smi)
	{
		using Float = float;
		const Float h = Float(0.5);

		const Vector2 c = smi.center;

		std::shared_ptr<RigidMesh> res;

		if (smi.wireframe)
		{
			std::vector<float> positions = {
				c.x-h,	 c.y-h,
				c.x-h,	 c.y+h,
				c.x+h,	 c.y+h,
				c.x+h,	 c.y-h,
			};

			std::vector<uint> indices = {
				0, 1,
				1, 2,
				2, 3, 
				3, 0,
			};

			res = std::make_shared<RigidMesh>(RigidMesh::CI{
				.app = smi.app,
				.name = smi.name,
				.dims = 2,
				.positions = std::move(positions),
				.indices = std::move(indices),
				.create_device_buffer = true,
			});
		}

		return res;
	}

	std::shared_ptr<RigidMesh> RigidMesh::MakeCube(CubeMakeInfo const& cmi)
	{
		using Float = float;
		const Float h = Float(0.5);
		const Float t = Float(1) / Float(3);
		const Float tt = Float(2) / Float(3);
		const Vector3 center = cmi.center;
		const Vector3 c = center;

		std::shared_ptr<RigidMesh> res;

		if (cmi.wireframe)
		{
			std::vector<float> positions = {
				c.x - h, c.y - h, c.z - h,
				c.x - h, c.y - h, c.z + h,
				c.x - h, c.y + h, c.z + h,
				c.x - h, c.y + h, c.z - h,

				c.x + h, c.y - h, c.z - h,
				c.x + h, c.y - h, c.z + h,
				c.x + h, c.y + h, c.z + h,
				c.x + h, c.y + h, c.z - h,
			};
			std::vector<uint> indices = {
				0, 1,
				1, 2,
				2, 3, 
				3, 0,

				4, 5,
				5, 6,
				6, 7,
				7, 4,

				0, 4,
				1, 5,
				2, 6,
				3, 7,
			};

			res = std::make_shared<RigidMesh>(RigidMesh::CI{
				.app = cmi.app,
				.name = cmi.name,
				.dims = 3,
				.positions = std::move(positions),
				.indices = std::move(indices),
				.create_device_buffer = true,
				.synch = cmi.synch,
			});
		}
		else
		{
			std::vector<Vertex> vertices;
			std::vector<uint> indices;


			if (cmi.face_normal)
			{
				const Vector3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
				const auto sign = [](Float f) {return f > 0 ? Float(1) : (f < 0 ? Float(-1) : Float(0)); };
				const auto addVertexes = [&](Float x, Float y, Float z, Float u, Float v)
				{
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(x) * X, .uv = Vector2{ u, v } });
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(y) * Y, .uv = Vector2{ u, v } });
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(z) * Z, .uv = Vector2{ u, v } });
				};

				const auto addVertexes3 = [&](Float x, Float y, Float z, Float u0, Float v0, Float u1, Float v1, Float u2, Float v2)
				{
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(x) * X, .uv = Vector2{ u0, v0 } });
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(y) * Y, .uv = Vector2{ u1, v1 } });
					vertices.emplace_back(Vertex{ .position = center + Vector3{ x, y, z }, .normal = sign(z) * Z, .uv = Vector2{ u2, v2 } });
				};

				if (cmi.same_face)
				{
					addVertexes3(-h, h, -h,		0, 0, 0, 1, 1, 0); // x0
					addVertexes3(h, h, -h,		1, 0, 1, 1, 0, 0); // x1
					addVertexes3(h, h, h,		0, 0, 1, 0, 1, 0); // x2
					addVertexes3(-h, h, h,		1, 0, 0, 0, 0, 0); // x3
					addVertexes3(-h, -h, -h,	0, 1, 0, 1, 1, 1); // x4
					addVertexes3(h, -h, -h,		1, 1, 0, 0, 0, 1); // x5
					addVertexes3(h, -h, h,		0, 1, 1, 0, 1, 1); // x6
					addVertexes3(-h, -h, h,		1, 1, 1, 1, 0, 1); // x7
				}
				else
				{

				}

				const auto id = [](uint xi, uint axis) {return xi * 3 + axis; };

				const auto addFace = [&](uint v0, uint v1, uint v2, uint v3, uint axis)
				{
					indices.push_back(id(v0, axis));
					indices.push_back(id(v1, axis));
					indices.push_back(id(v2, axis));
					indices.push_back(id(v2, axis));
					indices.push_back(id(v3, axis));
					indices.push_back(id(v0, axis));
				};
				addFace(0, 3, 2, 1, 1); // up (+y)
				addFace(0, 1, 5, 4, 2); // back (-z)
				addFace(0, 4, 7, 3, 0); // left (-x)
				addFace(6, 7, 4, 5, 1); // bottom (-y)
				addFace(6, 2, 3, 7, 2); // front (+z)
				addFace(6, 5, 1, 2, 0); // right (+x)
			}
			else
			{

			}

			res = std::make_shared<RigidMesh>(CreateInfo{
				.app = cmi.app, 
				.name = cmi.name,
				.vertices = std::move(vertices),
				.indices = std::move(indices),
				.auto_compute_tangents = true,
				.synch = cmi.synch,
			});

		}
		
		
		return res;
	}

	std::shared_ptr<RigidMesh> RigidMesh::MakeSphere(SphereMakeInfo const& smi)
	{
		const uint theta_divisions = smi.theta_divisions;
		const uint phi_divisions = smi.phi_divisions;
		const size_t num_vertices = (theta_divisions + 1) * (phi_divisions + 1);
		std::vector<Vertex> vertices(num_vertices);
		std::vector<uint> indices(num_vertices * 2 * 3);

		for (int t = 0; t <= theta_divisions; ++t)
		{
			const float v = float(t) / float(theta_divisions);
			const float theta = v * std::numbers::pi;
			const float ct = cos(theta);
			const float st = sin(theta);
			const float z = ct;


			const int base_line_index = t * (phi_divisions + 1);
			const int next_line_index = (t+1) * (phi_divisions + 1);

			for (int p = 0; p < phi_divisions; ++p)
			{
				const float u = float(p) / float(phi_divisions);
				const float phi = u * 2.0 * std::numbers::pi;

				const float cp = cos(phi);
				const float sp = sin(phi);

				const float x = st * cp;
				const float y = st * sp;

				const Vector3 normal = Vector3(x, z, y);
				vertices[base_line_index + p] = Vertex{
					.position = smi.center + smi.radius * normal,
					.normal = normal,
					.uv = Vector2(u, v),
				};

				if (t != theta_divisions)
				{
					const int base_index = 2 * 3 * (base_line_index + p);
					indices[base_index + 0] = base_line_index + p;
					indices[base_index + 1] = base_line_index + p + 1;
					indices[base_index + 2] = next_line_index + p + 1;

					indices[base_index + 3] = next_line_index + p + 1;
					indices[base_index + 4] = next_line_index + p;
					indices[base_index + 5] = base_line_index + p;
				}
			}

			vertices[base_line_index + phi_divisions] = vertices[base_line_index];
			vertices[base_line_index + phi_divisions].uv.x = 1.0;

		}

		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = smi.app,
			.name = smi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.auto_compute_tangents = true,
		});
		return res;
	}


	std::shared_ptr<RigidMesh> RigidMesh::MakeTetrahedron(PlatonMakeInfo const& pmi)
	{
		std::vector<Vertex> vertices;
		std::vector<uint> indices;

		const float r = pmi.radius;
		const vec3 c = pmi.center;

		if (pmi.face_normal)
		{
			vertices.reserve(3 * 4);
			
			vec3 v0 = vec3(r, r, r);
			vec3 v1 = vec3(r, -r, -r);
			vec3 v2 = vec3(-r, r, -r);
			vec3 v3 = vec3(-r, -r, r);

			const auto addFace = [&](vec3 v0, vec3 v1, vec3 v2)
			{
				vertices.push_back(Vertex{.position = c + v0});
				vertices.push_back(Vertex{.position = c + v1});
				vertices.push_back(Vertex{.position = c + v2});
			};
			
			addFace(v0, v1, v2);
			addFace(v1, v3, v2);
			addFace(v0, v2, v3);
			addFace(v0, v3, v1);
			
			indices.resize(3 * 4);
			std::iota(indices.begin(), indices.end(), 0);
		}
		else
		{
			vertices.resize(4);

			vertices[0].position = c + vec3(r, r, r);
			vertices[1].position = c + vec3(r, -r, -r);
			vertices[2].position = c + vec3(-r, r, -r);
			vertices[3].position = c + vec3(-r, -r, r);

			indices.reserve(3 * 4);

			const auto addFace = [&](uint i0, uint i1, uint i2)
			{
				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);
			};

			addFace(0, 1, 2);
			addFace(1, 3, 2);
			addFace(0, 2, 3);
			addFace(0, 3, 1);
		}

		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = pmi.app,
			.name = pmi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.compute_normals = 1,
			.auto_compute_tangents = false,
		});
		return res;
	}

	std::shared_ptr<RigidMesh> RigidMesh::MakeOctahedron(PlatonMakeInfo const& pmi)
	{
		const float phi = std::numbers::phi_v<float>;
		std::vector<Vertex> vertices;
		std::vector<uint> indices;

		const float r = pmi.radius;
		const vec3 c = pmi.center;

		if (pmi.face_normal)
		{
			
		}
		else
		{
			vertices.resize(6 + 3);

			vertices[0] = Vertex{
				.position = c + vec3(r, 0, 0),
				.uv = vec2(1, 0.5),
			};
			vertices[1] = Vertex{
				.position = c + vec3(0, r, 0),
				.uv = vec2(0.5, 0.5),
			};
			vertices[2] = Vertex{
				.position = c + vec3(0, 0, r),
				.uv = vec2(0.5, 1),
			};
			vertices[3] = Vertex{
				.position = c + vec3(-r, 0, 0),
				.uv = vec2(0, 0.5),
			};
			vertices[4] = Vertex{
				.position = c + vec3(0, -r, 0),
				.uv = vec2(0, 0),
			};
			vertices[5] = Vertex{
				.position = c + vec3(0, 0, -r),
				.uv = vec2(0.5 ,0),
			};

			vertices[6] = Vertex{
				.position = vertices[4].position,
				.uv = vec2(0, 1), 
			};

			vertices[7] = Vertex{
				.position = vertices[4].position,
				.uv = vec2(1, 0), 
			};

			vertices[8] = Vertex{
				.position = vertices[4].position,
				.uv = vec2(1, 1), 
			};


			indices.reserve(3 * 8);

			const auto addFace = [&](uint i0, uint i1, uint i2)
			{
				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);
			};

			addFace(0, 1, 2);
			addFace(1, 3, 2);
			addFace(0, 5, 1);
			addFace(1, 5, 3);

			addFace(0, 2, 8);
			addFace(6, 2, 3);
			addFace(0, 7, 5);
			addFace(4, 3, 5);

		}


		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = pmi.app,
			.name = pmi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.compute_normals = 1,
			.auto_compute_tangents = false,
		});

		res->_host.vertices[4].normal = vec3(0, -1, 0);
		res->_host.vertices[6].normal = vec3(0, -1, 0);
		res->_host.vertices[7].normal = vec3(0, -1, 0);
		res->_host.vertices[8].normal = vec3(0, -1, 0);

		return res;
	}

	std::shared_ptr<RigidMesh> RigidMesh::MakeIcosahedron(PlatonMakeInfo const& pmi)
	{
		const float phi = std::numbers::phi_v<float>;
		std::vector<Vertex> vertices;
		std::vector<uint> indices;


		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = pmi.app,
			.name = pmi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.auto_compute_tangents = true,
		});
		return res;
	}

	std::shared_ptr<RigidMesh> RigidMesh::MakeDodecahedron(PlatonMakeInfo const& pmi)
	{
		const float phi = std::numbers::phi_v<float>;
		std::vector<Vertex> vertices;
		std::vector<uint> indices;


		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = pmi.app,
			.name = pmi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.auto_compute_tangents = true,
		});
		return res;
	}



	
}