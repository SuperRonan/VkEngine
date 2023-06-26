#include "Mesh.hpp"
#include <numeric>
#include <numbers>

namespace vkl
{
	
	Mesh::Mesh(CreateInfo const& ci):
		VkObject(ci.app, ci.name)
	{}


	VertexInputDescription RigidMesh::vertexInputDesc()
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
		})
	{
		_host.vertices = ci.vertices;
		_host.indices32 = ci.indices;
		_host.index_type = VK_INDEX_TYPE_UINT32;
		_host.loaded = true;

		compressIndices();

		if (ci.auto_compute_tangents)
		{
			computeTangents();
		}
	}
	

	RigidMesh::~RigidMesh()
	{
		
	}

	void RigidMesh::compressIndices()
	{
		const bool index_uint8_t_available = false;// TODO activate the feature
		const size_t vs = _host.vertices.size();
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
				std::copy(tmp.begin(), tmp.end(), _host.indices8.begin());
				_device.up_to_date = false;
			}
			else if (idxt == VK_INDEX_TYPE_UINT32)
			{
				std::vector<u32> tmp = std::move(_host.indices32);
				_host.indices8 = std::vector<u8>(is);
				std::copy(tmp.begin(), tmp.end(), _host.indices8.begin());
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
				std::copy(tmp.begin(), tmp.end(), _host.indices16.begin());
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
		const size_t offset = _host.vertices.size();
		_host.vertices.resize(offset + other._host.vertices.size());
		std::copy(other._host.vertices.begin(), other._host.vertices.end(), _host.vertices.begin() + offset);
		
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

		DeviceData::Header header{
			.num_vertices = static_cast<uint32_t>(_host.vertices.size()),
			.num_indices = static_cast<uint32_t>(_host.indicesSize()),
			.num_primitives = static_cast<uint32_t>(_host.indicesSize() / 3),
			.vertices_per_primitive = 3,
		};

		_device.header_size = sizeof(header);
		_device.vertices_size = header.num_vertices * sizeof(Vertex);
		_device.indices_size = _host.indexBufferSize();
		_device.total_buffer_size = _device.header_size + _device.vertices_size + _device.indices_size;

		_device.num_indices = header.num_indices;
		_device.index_type = _host.index_type;
		
		_device.mesh_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = _app,
			.name = name() + ".mesh_buffer",
			.size = &_device.total_buffer_size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.queues = queues,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});
	}

	void RigidMesh::recordBindAndDraw(CommandBuffer& command_buffer)const
	{
		assert(_device.loaded());
		VkBuffer vb = *_device.mesh_buffer->instance();
		VkDeviceSize offset = _device.header_size;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &vb, &offset);
		vkCmdBindIndexBuffer(command_buffer, *_device.mesh_buffer->instance(), _device.vertices_size + _device.header_size, _device.index_type);

		vkCmdDrawIndexed(command_buffer, _device.num_indices, 1, 0, 0, 0);
	}

	Mesh::Status RigidMesh::getStatus() const
	{
		return Status{
			.host_loaded = _host.loaded,
			.device_loaded = _device.loaded(),
			.device_up_to_date = _device.up_to_date,
		};
	}

	bool RigidMesh::updateResources(UpdateContext& ctx)
	{
		bool res = false;
		
		if (!_device.loaded())
		{
			createDeviceBuffer({});
		}
		
		res |= _device.mesh_buffer->updateResource(ctx);

		return res;
	}

	Mesh::ResourcesToUpload RigidMesh::getResourcesToUpload()
	{
		assert(_device.loaded());
		ResourcesToUpload res;

		std::vector<PositionedObjectView> sources(3);
		
		DeviceData::Header header{
			.num_vertices = static_cast<uint32_t>(_host.vertices.size()),
			.num_indices = static_cast<uint32_t>(_host.indicesSize()),
			.num_primitives = static_cast<uint32_t>(_host.indicesSize() / 3),
			.vertices_per_primitive = 3,
		};

		sources[0] = PositionedObjectView{
			.obj = header,
			.pos = 0,
		};

		sources[1] = PositionedObjectView{
			.obj = _host.vertices,
			.pos = _device.header_size,
		};

		sources[2] = PositionedObjectView{
			.obj = _host.indicesView(),
			.pos = _device.header_size + _device.vertices_size,
		};

		res.buffers.push_back(ResourcesToUpload::BufferUpload{
			.sources = sources,
			.dst = _device.mesh_buffer,
		});
		return res;
	}


	std::shared_ptr<RigidMesh> RigidMesh::MakeCube(CubeMakeInfo const& cmi)
	{
		using Float = float;
		const Float h = Float(0.5);
		const Float t = Float(1) / Float(3);
		const Float tt = Float(2) / Float(3);

		std::vector<Vertex> vertices;
		std::vector<uint> indices;

		const Vector3 center = cmi.center;

		if (cmi.face_normal)
		{
			Vector3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
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

		std::shared_ptr<RigidMesh> res = std::make_shared<RigidMesh>(CreateInfo{
			.app = cmi.app, 
			.name = cmi.name,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
			.auto_compute_tangents = true,
		});
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
}