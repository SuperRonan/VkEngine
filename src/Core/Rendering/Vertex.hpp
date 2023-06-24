#pragma once

#include <Core/VulkanCommons.hpp>
#include <glm/glm.hpp>
#include <array>
#include <type_traits>

namespace vkl
{
	struct Vertex
	{
		using Vector4 = glm::vec4;
		using Vector3 = glm::vec3;
		using Vector2 = glm::vec2;
		using Matrix4 = glm::mat4;
		using Matrix3 = glm::mat3;

		Vector3 position;
		Vector3 normal;
		Vector3 tangent;
		Vector2 uv;

		void transform(Matrix4 const& m, Matrix3 const& nm) noexcept;

		void transform(Matrix4 const& m) noexcept;

		Vertex& operator*=(Matrix4 const& m) noexcept;

		static constexpr VkVertexInputBindingDescription getBindingDescription() noexcept;
		
		static constexpr std::array<VkVertexInputAttributeDescription, 4> getAttributeDescription() noexcept;
	};

	
}