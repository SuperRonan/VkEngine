#include <vkl/Rendering/Vertex.hpp>

#include <vkl/Maths/Transforms.hpp>

namespace vkl
{
	void Vertex::transform(Matrix4 const& m, Matrix3 const& nm) noexcept
	{
		position = m * Vector4(Vector3(position), 1);
		normal =  Vector4(Normalize(nm * Vector3(normal)), 0.0f);
		tangent = Vector4(Normalize(nm * Vector3(tangent)), 0.0f);
	}

	void Vertex::transform(Matrix4 const& m) noexcept
	{
		Matrix3 nm = DirectionMatrix(Matrix3(m));
		transform(m, nm);
	}

	Vertex& Vertex::operator*=(Matrix4 const& m) noexcept
	{
		transform(m);
		return *this;
	}

	constexpr VkVertexInputBindingDescription Vertex::getBindingDescription() noexcept
	{
		VkVertexInputBindingDescription res{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		return res;
	}

	constexpr std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescription() noexcept
	{
		std::array<VkVertexInputAttributeDescription, 4> res;
		res[0] = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, position),
		};
		res[1] = {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, normal),
		};
		res[2] = {
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, tangent),
		};
		res[3] = {
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, uv),
		};
		return res;
	}
}