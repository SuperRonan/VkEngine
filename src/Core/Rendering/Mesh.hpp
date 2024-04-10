#pragma once

#include <Core/App/VkApplication.hpp>

#include "Geometry.hpp"
#include "Vertex.hpp"

#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/CommandBuffer.hpp>

#include <Core/Execution/ResourcesHolder.hpp>

#include <Core/Rendering/Drawable.hpp>

#include <Core/IO/GuiContext.hpp>

#include <Core/Maths/AlignedAxisBoundingBox.hpp>
#include <Core/VkObjects/AccelerationStructure.hpp>

#include <vector>

namespace vkl
{
	
	struct MeshHeader
	{
		uint32_t num_vertices = 0;
		uint32_t num_indices = 0;
		uint32_t num_primitives = 0;
		uint32_t flags = 0;
	};

#define MESH_FLAG_INDEX_TYPE_UINT16 0
#define MESH_FLAG_INDEX_TYPE_UINT32 1
#define MESH_FLAG_INDEX_TYPE_UINT8  2
#define MESH_FLAG_INDEX_TYPE_MASK	3

	inline uint32_t meshFlags(VkIndexType index_type)
	{
		uint32_t res = 0;
		switch (index_type)
		{
		case VK_INDEX_TYPE_UINT16:
			res |= MESH_FLAG_INDEX_TYPE_UINT16;
		break;
		case VK_INDEX_TYPE_UINT32:
			res |= MESH_FLAG_INDEX_TYPE_UINT32;
		break;
		case VK_INDEX_TYPE_UINT8_EXT:
			res |= MESH_FLAG_INDEX_TYPE_UINT8;
		break;
		}
		return res;
	}

	class Pipeline;

	class Mesh : public VkObject, public Geometry, public Drawable
	{
	public:
		enum class Type : uint32_t
		{
			None = 0,
			Rigid = 1,
		};
	protected:
		
		using u32 = uint32_t;
		using u16 = uint16_t;
		using u8 = uint8_t;
		using uint = u32;

		using Vector2 = glm::vec2;
		using Vector3 = glm::vec3;
		using Vector4 = glm::vec4;
		using Matrix3 = glm::mat3;
		using Matrix4 = glm::mat4;

		using vec2 = Vector2;
		using vec3 = Vector3;
		using vec4 = Vector4;
		using mat4 = Matrix4;
		using mat3 = Matrix3;

		Type _type = Type::None;
		bool _is_synch = true;

		AABB3f _aabb;

		std::shared_ptr<BottomLevelAccelerationStructure> _blas = nullptr;

		MyVector<DescriptorSetAndPool::Registration> _registered_sets = {};

	public:

		

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Type type = Type::None;
			bool synch = true;
		};
		using CI = CreateInfo;

		Mesh(CreateInfo const& ci);

		virtual ~Mesh() = default;

		constexpr Type type()const
		{
			return _type;
		}

		constexpr bool isSynch()const
		{
			return _is_synch;
		}
		
		struct Status
		{
			bool host_loaded = false;
			bool device_loaded = false;
			bool device_up_to_date = false;
		};

		virtual Status getStatus()const = 0;

		virtual VertexInputDescription vertexInputDesc() override = 0;

		virtual void updateResources(UpdateContext & ctx) = 0;

		virtual void fillVertexDrawCallInfo(VertexDrawCallInfo& vr) override = 0;

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) = 0;

		virtual ShaderBindings getShaderBindings(uint offset) = 0;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0) = 0;

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set) = 0;

		virtual void callResourceUpdateCallbacks() = 0;

		virtual bool isReadyToDraw() const = 0;

		virtual void declareGui(GuiContext & ctx) = 0;

		const AABB3f& getAABB()const
		{
			return _aabb;
		}

		std::shared_ptr<BLAS> const& blas()const
		{
			return _blas;
		}
	};

	class RigidMesh : public Mesh
	{
	protected:

		struct HostData
		{
			bool loaded = false;
			bool use_full_vertices = true;
			uint8_t dims = 3;
			// Can be 2D or 3D
			std::vector<float> positions;
			std::vector<Vertex> vertices;
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;
			union {
				std::vector<u32> indices32 = {};
				std::vector<u16> indices16;
				std::vector<u8>  indices8;
			};

			HostData() = default;

			~HostData()
			{
				if (index_type == VK_INDEX_TYPE_UINT32)
				{
					indices32.clear();
					indices32.shrink_to_fit();
					indices32.~vector();
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					indices16.clear();
					indices16.shrink_to_fit();
					indices16.~vector();
				}
				else if (index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					indices8.clear();
					indices8.shrink_to_fit();
					indices8.~vector();
				}
			}

			size_t numVertices() const
			{
				return use_full_vertices ? vertices.size() : (positions.size() / dims);
			}

			uint32_t getIndex(size_t i)const
			{
				if (index_type == VK_INDEX_TYPE_UINT32)
				{
					return indices32[i];
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					return indices16[i];
				}
				else if (index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					return indices8[i];
				}
				else
				{
					return 0;
				}
			}

			size_t indicesSize() const
			{
				if (index_type == VK_INDEX_TYPE_UINT32)
				{
					return indices32.size();
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					return indices16.size();
				}
				else if(index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					return indices8.size();
				}
				else
				{
					return 0;
				}
			}

			size_t indexSize() const 
			{
				if (index_type == VK_INDEX_TYPE_UINT32)
				{
					return 4;
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					return 2;
				}
				else if (index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}

			size_t indexBufferSize() const
			{
				return indicesSize() * indexSize();
			}

			const void* indicesData() const
			{
				if (index_type == VK_INDEX_TYPE_UINT32)
				{
					return (const void*)indices32.data();
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					return (const void*)indices16.data();
				}
				else if (index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					return (const void*)indices8.data();
				}
				else
				{
					return nullptr;
				}
			}

			ObjectView indicesView() const
			{
				return ObjectView(indicesData(), indexBufferSize());
			}

		} _host;

		struct DeviceData
		{
			uint32_t num_indices = 0;
			uint32_t num_vertices = 0;
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;

			std::shared_ptr<Buffer> mesh_buffer = nullptr;
			// In bytes
			VkDeviceSize header_size = 0, vertices_size = 0, indices_size = 0;
			VkDeviceSize total_buffer_size = 0;
			BufferAndRange header_buffer;
			BufferAndRange vertex_buffer;
			BufferAndRange index_buffer;
			// Structure of the mesh buffer:
			// header
			// vertices
			// indices
			bool up_to_date = false;
			bool uploaded = false;
			bool just_uploaded = false;
			uint8_t index_type_size = 0;

			bool loaded()const
			{
				return !!mesh_buffer;
			}

		} _device;

		bool checkIntegrity() const;

		void transmitToRegisteredSet(DescriptorSetAndPool::Registration & rs);

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			uint8_t dims = 3;
			std::vector<float> positions = {};
			std::vector<Vertex> vertices = {};
			std::vector<uint> indices = {};
			int compute_normals = 0;
			bool auto_compute_tangents = false;
			bool create_device_buffer = true;
			bool synch = true;
		};
		using CI = CreateInfo;

		RigidMesh(CreateInfo const& ci);

		virtual ~RigidMesh();

		MeshHeader getHeader() const;

		void compressIndices();

		void decompressIndices();

		void merge(RigidMesh const& other) noexcept;

		RigidMesh& operator+=(RigidMesh const& other) noexcept;

		void transform(Matrix4 const& m);

		RigidMesh& operator*=(Matrix4 const& m);

		void computeTangents();

		void computeNormals(int mode);

		void computeAABB();

		void flipFaces();

		void createDeviceBuffer(std::vector<uint32_t> const& queues);

		virtual Status getStatus() const override;

		virtual void fillVertexDrawCallInfo(VertexDrawCallInfo& vr) override;

		static std::vector<DescriptorSetLayout::Binding> getSetLayoutBindingsStatic(uint32_t offset);

		virtual std::vector<DescriptorSetLayout::Binding> getSetLayoutBindings(uint32_t offset) override final
		{
			return getSetLayoutBindingsStatic(offset);
		}

		virtual ShaderBindings getShaderBindings(uint offset) override final;

		virtual void updateResources(UpdateContext & ctx) override;

		virtual void registerToDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set, uint32_t binding, uint32_t array_index = 0) override;

		virtual void unRegistgerFromDescriptorSet(std::shared_ptr<DescriptorSetAndPool> const& set) override;

		virtual void callResourceUpdateCallbacks() override;

		virtual bool isReadyToDraw() const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual VertexInputDescription vertexInputDesc() override
		{
			if (_host.use_full_vertices)
			{
				return vertexInputDescFullVertex();
			}
			else
			{
				if (_host.dims == 3)
				{
					return vertexInputDescOnlyPos3D();
				}
				else if (_host.dims == 2)
				{
					return vertexInputDescOnlyPos2D();
				}
			}
			return {};
		}

		static VertexInputDescription vertexInputDescFullVertex();
		static VertexInputDescription vertexInputDescOnlyPos3D();
		static VertexInputDescription vertexInputDescOnlyPos2D();

		std::shared_ptr<Buffer> vertexIndexBuffer()const
		{
			return _device.mesh_buffer;
		}

		struct Square2DMakeInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Vector2 center = Vector2(0);
			bool wireframe = false;
		};
		static std::shared_ptr<RigidMesh> MakeSquare(Square2DMakeInfo const& smi);

		struct CubeMakeInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Vector3 center = Vector3(0);

			bool wireframe = false;
			bool face_normal = true;
			bool same_face = true;

			bool synch = true;
		};

		static std::shared_ptr<RigidMesh> MakeCube(CubeMakeInfo const& cmi);

		struct SphereMakeInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Vector3 center = Vector3(0);
			float radius = 1;
			uint theta_divisions = 16;
			uint phi_divisions = 32;
		};

		static std::shared_ptr<RigidMesh> MakeSphere(SphereMakeInfo const& smi);

		struct PlatonMakeInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Vector3 center = Vector3(0);
			float radius = 1;
			int position = 1;
			bool face_normal = false;
		};

		static std::shared_ptr<RigidMesh> MakeTetrahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeOctahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeIcosahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeDodecahedron(PlatonMakeInfo const& pmi);
	};
}