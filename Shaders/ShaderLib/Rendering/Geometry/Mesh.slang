#pragma once

#include "Geometry.slang"
#include "MeshDefinitions.h"
#include <ShaderLib/Maths/AffineXForm.slang>
#include <ShaderLib/Maths/Calculus.slang>

#include <ShaderLib/Debug/DebugBuffers.slang>

interface IMesh : IGeometry<float, 3>
{



	template <int N>
	uvec<N> getPrimitiveIndices(uint i);

};

struct MeshBase
{

};

template <typename T, typename Array_T : IArray<T>, int N>
Array<T, N> MultiRead(const in Array_T array, uvec<N> indices)
{
	Array<T, N> res;
	for(uint i =0; i < N; ++i)
	{
		res[i] = array[indices[i]];
	}
	return res;
}


float Proj<int N>(vec<N> p, vec<N> v)
{
	return Dot(v, p) / Length2(p);
}

struct Mesh : MeshBase
{
	struct Header
	{
		uint num_vertices;
		uint num_indices;
		uint num_primitives;
		uint flags;
		AABB3f box;
	};

	Header header;
	
	struct Vertex
	{
		vec3 position;
		vec3 normal;
		vec3 tangent;
		vec2 uv;

		[mutating]
		void normalize()
		{
			normal = Normalize(normal);
			tangent = Normalize(tangent);
		}

		Vertex normalized()
		{
			Vertex res = this;
			res.normalize();
			return res;
		}

		[mutating]
		void transformBy(const in AffineXForm3Df xform, const in Matrix3f dir_xform)
		{
			position = xform * Homogeneous(position);
			normal = dir_xform * normal;
			tangent = xform * vec4(tangent, 0);
		}

		[mutating]
		void transformBy(const in AffineXForm3Df xform)
		{
			transformBy(xform, DirectionMatrix(xform));
		}

		Vertex transformedBy(const in AffineXForm3Df xform, const Matrix3f dir_xform)
		{
			Vertex res = this;
			res.transformBy(xform, dir_xform);
			return res;
		}

		Vertex transformedBy(const in AffineXForm3Df xform)
		{
			return transformedBy(xform, DirectionMatrix(xform));
		}

	};

	AABB3f getAABB()
	{
		return header.box;
	}

	StructuredBuffer<Vertex, Std430DataLayout> _vertices;
	ByteAddressBuffer _indices;

	template <UInt : __BuiltinIntegerType, int N>
	static Vector<UInt, N> ReadIndicesWithFormat(const in ByteAddressBuffer buffer, uint i)
	{
		Vector<UInt, N> res;
		// TODO check that the load works (since it has a smaller granularity than u32)
		res = buffer.Load<Vector<UInt, N>>(sizeof(UInt) * N * i);
		return res;
	}

	template <int N>
	static uvec<N> ReadIndices(const in ByteAddressBuffer buffer, uint i, uint flags)
	{
		uvec<N> res;
		const uint index_type = flags & MESH_FLAG_INDEX_TYPE_MASK;
		if(index_type == MESH_FLAG_INDEX_TYPE_UINT16)
		{
			res = uvec<N>(ReadIndicesWithFormat<u16, N>(buffer, i));
		}
		else if(index_type == MESH_FLAG_INDEX_TYPE_UINT32)
		{
			res = uvec<N>(ReadIndicesWithFormat<u32, N>(buffer, i));
		}
		else if(index_type == MESH_FLAG_INDEX_TYPE_UINT8)
		{
			res = uvec<N>(ReadIndicesWithFormat<u8, N>(buffer, i));
		}
		else
		{
			for(uint j = 0; j < N; ++j)
			{
				res[j] = i * N + j;
			}
		}
		return res;
	}

	template <UInt : __BuiltinIntegerType, int N>
	Vector<UInt, N> readIndicesWithFormat(uint i)
	{
		return ReadIndicesWithFormat<UInt, N>(_indices, i);
	}

	template <int N>
	uvec<N> getPrimitiveIndices(uint i)
	{
		return ReadIndices<N>(_indices, i, header.flags);
	}

	template <int N>
	static Vertex InterpolateVertex(const in IArray<Vertex> vertices, uvec<N> indices, Vectorf<N> weights)
	{
		Vertex res = {};
		res.position = 0;
		res.normal = 0;
		res.tangent = 0;
		res.uv = 0;
		for(uint i = 0; i < N; ++i)
		{
			const Vertex v = vertices[indices[i]];
			const float w = weights[i];
			res.position += w * v.position;
			res.normal += w * v.normal;
			res.tangent += w * v.tangent;
			res.uv += w * v.uv;
		}
		return res;
	}

	template <int N>
	Vertex interpolateVertex(uvec<N> indices, Vectorf<N> weights)
	{
		return InterpolateVertex(_vertices, indices, weights);
	}

	template <int N>
	Vertex interpolateVertex(uvec<N> indices, Vectorf<N - 1> uvs)
	{
		return interpolateVertex(indices, UVToBarycentric(uvs));
	}

	// Does NOT return a normalized vector
	static vec3 ReadTriangleNormal(const in IArray<Vertex> vertices, const in uvec3 indices, const in Matrix3f xform)
	{
		return Cross(
			xform * (vertices[indices[1]].position - vertices[indices[0]].position),
			xform * (vertices[indices[2]].position - vertices[indices[0]].position)
		);
	}

	// Does NOT return a normalized vector
	static vec3 ReadTriangleNormal(const in IArray<Vertex> vertices, const in uvec3 indices)
	{
		return ReadTriangleNormal(vertices, indices, DiagonalMatrix<3, 3>(1.0f));
	}

	static SurfaceGeometryDifferentials<3> GetTriangleDifferentials(
		const in Matrix3f object_to_world,
		const in IArray<Vertex> vertices_buffer, 
		const in uvec3 indices)
	{
		SurfaceGeometryDifferentials<3> res = {};
		const Array<Vertex, 3> vertices = MultiRead(vertices_buffer, indices);
		Matrix3f planes;
		Matrix2x3f textures;
		Matrix3f normals;
		for(uint i = 0; i < 3; ++i)
		{
			vec3 vertex = vertices[i].position;
			vec3 v1 = vertices[(i + 1) % 3].position;
			vec3 v2 = vertices[(i + 2) % 3].position;
			vec3 edge = v2 - v1;
			vec3 altitude_base = Proj(edge, vertex - v1) * edge + v1;
			vec3 normal_dir = (vertex - altitude_base);
			vec3 plane = normal_dir / Length2(normal_dir);
			SetRow(planes, i, plane);
			SetColumn(textures, i, vertices[i].uv);
			SetColumn(normals, i, object_to_world * vertices[i].normal);
		}
		let barycentrics_jacobians = planes;
		res.Juv = textures * barycentrics_jacobians;
		res.Jnormal = normals * barycentrics_jacobians;
		return res;
	}
};

#if BIND_SINGLE_MESH

layout(MESH_BINDING_BASE + 0) restrict StructuredBuffer<Mesh::Header, Std430DataLayout> BoundMeshHeader;
layout(MESH_BINDING_BASE + 1) restrict StructuredBuffer<Mesh::Vertex, Std430DataLayout> BoundMeshVertices;
layout(MESH_BINDING_BASE + 2) restrict ByteAddressBuffer BoundMeshIndices;

struct SingleBoundMesh : Mesh, IMesh
{
	

	__init()
	{
		// TODO
	}
};

#endif