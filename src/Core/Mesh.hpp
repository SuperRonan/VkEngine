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

	public:

		Mesh(VkApplication * app);
		Mesh(VkApplication* app, std::vector<Vertex> const& vertices, std::vector<uint> const& indices) noexcept;
		Mesh(VkApplication* app, std::vector<Vertex> && vertices, std::vector<uint> && indices) noexcept;

		Mesh(Mesh const&);
		Mesh(Mesh&& mesh) noexcept;
		Mesh& operator=(Mesh const&);
		Mesh& operator=(Mesh&&) noexcept;

		~Mesh();

		void merge(Mesh const& other) noexcept;

		Mesh& operator+=(Mesh const& other) noexcept;

		void transform(Matrix4 const& m);

		Mesh& operator*=(Matrix4 const& m);

		void computeTangents();

		void createDeviceBuffer(std::vector<uint32_t> const& queues);

		void copyToDevice();

		void cleanDeviceBuffer();

		void recordBind(VkCommandBuffer command_buffer, uint32_t first_binding = 0)const;

		uint32_t deviceIndexSize()const;

		constexpr auto indicesSize()const
		{
			return _device.indices_size;
		}

		static Mesh Cube(VkApplication * app, Vector3 center=Vector3(0), bool face_normal=true, bool same_face=true);
	};
}