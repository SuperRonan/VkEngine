#include "Mesh.hpp"
#include "StagingPool.hpp"
#include <numeric>
#include <numbers>

namespace vkl
{
	VertexInputDescription Mesh::vertexInputDesc()
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

	bool Mesh::DeviceData::loaded()const
	{
		return !!mesh_buffer;
	}

	Mesh::Mesh(VkApplication* app):
		VkObject(app)
	{}

	Mesh::Mesh(VkApplication* app, std::vector<Vertex> const& vertices, std::vector<uint> const& indices) noexcept:
		VkObject(app)
	{
		_host = {
			.loaded = true,
			.vertices = vertices,
			.indices = indices,
		};
	}

	Mesh::Mesh(VkApplication* app, std::vector<Vertex> && vertices, std::vector<uint> && indices) noexcept:
		VkObject(app)
	{
		_host = {
			.loaded = true,
			.vertices = std::move(vertices),
			.indices = std::move(indices),
		};
	}

	Mesh::~Mesh() 
	{
		if (_device.loaded())
		{
			cleanDeviceBuffer();
		}
	}

	void Mesh::merge(Mesh const& other) noexcept
	{
		size_t offset = _host.vertices.size();
		size_t elem_size = sizeof(typename decltype(_host.vertices)::value_type);
		_host.vertices.resize(offset + other._host.vertices.size());
		std::copy(other._host.vertices.begin(), other._host.vertices.end(), _host.vertices.begin() + offset);
		size_t n_indices = _host.indices.size();
		_host.indices.resize(n_indices + other._host.indices.size());
		std::iota(_host.indices.begin() + n_indices, _host.indices.end(), (uint)offset);
	}

	Mesh& Mesh::operator+=(Mesh const& other) noexcept
	{
		merge(other);
		return *this;
	}

	void Mesh::transform(Matrix4 const& m)
	{
		const Matrix3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
		for (Vertex& vertex : _host.vertices)
		{
			vertex.transform(m, nm);
		}
	}

	Mesh& Mesh::operator*=(Matrix4 const& m)
	{
		transform(m);
		return *this;
	}

	void Mesh::computeTangents()
	{
		using Float = float;
		// https://marti.works/posts/post-calculating-tangents-for-your-mesh/post/
		// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
		uint number_of_triangles = _host.indices.size() / 3;

		struct tan
		{
			Vector3 tg, btg;
		};

		std::vector<tan> tans(_host.vertices.size(), tan{ Vector3(0), Vector3(0) });

		for (uint triangle_index = 0; triangle_index < number_of_triangles; ++triangle_index)
		{
			uint i0 = _host.indices[triangle_index * 3];
			uint i1 = _host.indices[triangle_index * 3 + 1];
			uint i2 = _host.indices[triangle_index * 3 + 2];

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
		tans.resize(0);
		tans.shrink_to_fit();
	}

	void Mesh::createDeviceBuffer(std::vector<uint32_t> const& queues)
	{
		assert(_host.loaded);
		assert(!_device.loaded());

		_device.vertices_size = _host.vertices.size() * sizeof(Vertex), _device.indices_size = _host.indices.size() * sizeof(uint);
		_device.mesh_size = _device.vertices_size + _device.indices_size;

		
		_device.mesh_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = _app,
			.name = name() + ".mesh_buffer",
			.size = _device.mesh_size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.queues = queues,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.create_on_construct = true,
		});
	}

	//void Mesh::copyToDevice()
	//{
	//	assert(_device.loaded());
	//	assert(_host.loaded);
	//	StagingPool& sp = _app->stagingPool();
	//	StagingPool::StagingBuffer* sb = sp.getStagingBuffer(_device.mesh_size);
	//		std::memcpy(sb->data, _host.vertices.data(), _device.vertices_size);
	//		std::memcpy((char *)sb->data + _device.vertices_size, _host.indices.data(), _device.indices_size);
	//	sp.releaseStagingBuffer(sb);
	//}

	void Mesh::cleanDeviceBuffer()
	{
		assert(_device.loaded());
		_device = DeviceData();
	}

	void Mesh::recordBind(VkCommandBuffer command_buffer, uint32_t first_binding)const
	{
		assert(_device.loaded());
		VkBuffer vb = *_device.mesh_buffer->instance();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(command_buffer, first_binding, 1, &vb, &offset);
		vkCmdBindIndexBuffer(command_buffer, *_device.mesh_buffer->instance(), _device.vertices_size, VK_INDEX_TYPE_UINT32);
	}


	std::shared_ptr<Mesh> Mesh::MakeCube(VkApplication * app, Vector3 center, bool face_normal, bool same_face)
	{
		using Float = float;
		const Float h = Float(0.5);
		const Float t = Float(1) / Float(3);
		const Float tt = Float(2) / Float(3);

		std::vector<Vertex> vertices;
		std::vector<uint> indices;

		if (face_normal)
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

			if (same_face)
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

		std::shared_ptr<Mesh> res = std::make_shared<Mesh>(app, std::move(vertices), std::move(indices));
		res->computeTangents();
		return res;
	}

	std::shared_ptr<Mesh> Mesh::MakeSphere(SphereMakeInfo const& smi)
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

				vertices[base_line_index + p] = Vertex{
					.position = smi.center + smi.radius * Vector3(x, y, z),
					.normal = Vector3(x, y, z),
					.uv = Vector2(u, v),
				};

				const int base_index = 2 * 3 * (base_line_index + p);
				indices[base_index + 0] = base_line_index + p;
				indices[base_index + 1] = base_line_index + p + 1;
				indices[base_index + 2] = next_line_index + p + 1;

				indices[base_index + 3] = next_line_index + p + 1;
				indices[base_index + 4] = next_line_index + p;
				indices[base_index + 5] = base_line_index + p;
			}

			vertices[base_line_index + phi_divisions] = vertices[base_line_index];
			vertices[base_line_index + phi_divisions].uv.x = 1.0;

		}

		std::shared_ptr<Mesh> res = std::make_shared<Mesh>(smi.app, std::move(vertices), std::move(indices));
		//res->computeTangents();
		return res;
	}
}