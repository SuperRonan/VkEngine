#include "Vertex.hpp"

namespace vkl
{
	void Vertex::transform(Matrix4 const& m, Matrix3 const& nm) noexcept
	{
		position = Vector3(m * Vector4(position, 1));
		normal = glm::normalize(nm * normal);
		tangent = glm::normalize(nm * tangent);
	}

	void Vertex::transform(Matrix4 const& m) noexcept
	{
		Matrix3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
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