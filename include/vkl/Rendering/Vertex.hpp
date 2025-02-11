#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <array>
#include <type_traits>

#include <vkl/Maths/Types.hpp>

namespace vkl
{
	struct Vertex
	{
		using Vector4 = Vector4f;
		using Vector3 = Vector3f;
		using Vector2 = Vector2f;
		using Matrix4 = Matrix4f;
		using Matrix3 = Matrix3f;

		Vector4 position;
		Vector4 normal;
		Vector4 tangent;
		Vector4 uv;

		void transform(Matrix4 const& m, Matrix3 const& nm) noexcept;

		void transform(Matrix4 const& m) noexcept;

		Vertex& operator*=(Matrix4 const& m) noexcept;

		static constexpr VkVertexInputBindingDescription getBindingDescription() noexcept;
		
		static constexpr std::array<VkVertexInputAttributeDescription, 4> getAttributeDescription() noexcept;
	};

	
}