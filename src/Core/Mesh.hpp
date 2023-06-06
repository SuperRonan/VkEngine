#pragma once

#include "VkApplication.hpp"
#include "Geometry.hpp"
#include <vector>
#include "Vertex.hpp"
#include "Buffer.hpp"


namespace vkl
{
	class Mesh : public VkObject, public Geometry 
	{
	protected:

		using uint = uint32_t;
		using Vector2 = glm::vec2;
		using Vector3 = glm::vec3;
		using Vector4 = glm::vec4;
		using Matrix3 = glm::mat3;
		using Matrix4 = glm::mat4;

		struct HostData
		{
			bool loaded = false;
			std::vector<Vertex> vertices;
			std::vector<uint> indices;
		} _host;

		struct DeviceData
		{
			std::shared_ptr<Buffer> mesh_buffer = nullptr;
			// In bytes
			VkDeviceSize vertices_size = 0, indices_size = 0, mesh_size = 0;

			bool loaded()const;
		} _device;

		bool checkIntegrity() const;

	public:

		Mesh(VkApplication * app);
		Mesh(VkApplication* app, std::vector<Vertex> const& vertices, std::vector<uint> const& indices) noexcept;
		Mesh(VkApplication* app, std::vector<Vertex> && vertices, std::vector<uint> && indices) noexcept;

		~Mesh();

		void merge(Mesh const& other) noexcept;

		Mesh& operator+=(Mesh const& other) noexcept;

		void transform(Matrix4 const& m);

		Mesh& operator*=(Matrix4 const& m);

		void computeTangents();

		void createDeviceBuffer(std::vector<uint32_t> const& queues);

		void cleanDeviceBuffer();

		void recordBind(VkCommandBuffer command_buffer, uint32_t first_binding = 0)const;

		static VertexInputDescription vertexInputDesc();

		const std::vector<Vertex>& vertices() const
		{
			return _host.vertices;
		}

		const std::vector<uint>& indices() const
		{
			return _host.indices;
		}

		std::shared_ptr<Buffer> combinedBuffer()const
		{
			return _device.mesh_buffer;
		}

		constexpr auto indicesLength()const
		{
			return _device.indices_size / sizeof(uint32_t);
		}

		static std::shared_ptr<Mesh> MakeCube(VkApplication * app, Vector3 center=Vector3(0), bool face_normal=true, bool same_face=true);

		struct SphereMakeInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Vector3 center = Vector3(0);
			float radius = 1;
			uint theta_divisions = 16;
			uint phi_divisions = 32;
		};
		using SMI = SphereMakeInfo;
		static std::shared_ptr<Mesh> MakeSphere(SphereMakeInfo const& smi);
	};
}