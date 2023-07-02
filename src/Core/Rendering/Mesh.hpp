#pragma once

#include <Core/App/VkApplication.hpp>
#include "Geometry.hpp"
#include <vector>
#include "Vertex.hpp"
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/CommandBuffer.hpp>

namespace vkl
{
	struct Resources
	{
		std::vector<std::shared_ptr<Buffer>> buffers = {};
		std::vector<std::shared_ptr<ImageView>> images = {};
	};

	class Mesh : public VkObject, public Geometry
	{
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


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		Mesh(CreateInfo const& ci);

		virtual ~Mesh() = default;
		
		struct Status
		{
			bool host_loaded = false;
			bool device_loaded = false;
			bool device_up_to_date = false;
		};

		virtual Status getStatus()const = 0;
		
		virtual void recordBindAndDraw(CommandBuffer & cmd)const = 0;

		virtual Resources getResources() = 0;

		virtual bool updateResources(UpdateContext & ctx) = 0;

		struct ResourcesToUpload
		{
			struct ImageUpload
			{
				ObjectView src;
				std::shared_ptr<ImageView> dst;
			};

			std::vector<ImageUpload> images;

			struct BufferUpload
			{
				std::vector<PositionedObjectView> sources;
				std::shared_ptr<Buffer> dst;
			};
			
			std::vector<BufferUpload> buffers;
		};

		virtual ResourcesToUpload getResourcesToUpload() = 0;

		virtual void notifyDeviceDataIsUpToDate() = 0;

	};

	class RigidMesh : public Mesh
	{
	protected:


		struct HostData
		{
			bool loaded = false;
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
				}
				else if (index_type == VK_INDEX_TYPE_UINT16)
				{
					indices16.clear();
					indices16.shrink_to_fit();
				}
				else if (index_type == VK_INDEX_TYPE_UINT8_EXT)
				{
					indices8.clear();
					indices8.shrink_to_fit();
				}
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
			struct Header
			{
				uint32_t num_vertices = 0;
				uint32_t num_indices = 0;
				uint32_t num_primitives = 0;
				uint32_t vertices_per_primitive = 0;
			};

			uint32_t num_indices = 0;
			VkIndexType index_type = VK_INDEX_TYPE_MAX_ENUM;

			std::shared_ptr<Buffer> mesh_buffer = nullptr;
			// In bytes
			VkDeviceSize header_size = 0, vertices_size = 0, indices_size = 0;
			VkDeviceSize total_buffer_size = 0;
			// Structure of the mesh buffer:
			// header
			// vertices
			// indices
			bool up_to_date = false;


			bool loaded()const
			{
				return !!mesh_buffer;
			}

		} _device;

		bool checkIntegrity() const;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			std::vector<Vertex> vertices = {};
			std::vector<uint> indices = {};
			int compute_normals = 0;
			bool auto_compute_tangents = false;
		};
		using CI = CreateInfo;

		RigidMesh(CreateInfo const& ci);

		virtual ~RigidMesh();

		void compressIndices();

		void decompressIndices();

		void merge(RigidMesh const& other) noexcept;

		RigidMesh& operator+=(RigidMesh const& other) noexcept;

		void transform(Matrix4 const& m);

		RigidMesh& operator*=(Matrix4 const& m);

		void computeTangents();

		void computeNormals(int mode);

		void flipFaces();

		void createDeviceBuffer(std::vector<uint32_t> const& queues);

		virtual Status getStatus() const override;

		virtual void recordBindAndDraw(CommandBuffer & cmd)const override;

		virtual Resources getResources() override
		{
			return Resources{
				.buffers = {_device.mesh_buffer},
			};
		}

		virtual bool updateResources(UpdateContext & ctx) override;

		virtual void notifyDeviceDataIsUpToDate() override
		{
			_device.up_to_date = true;
		}

		virtual ResourcesToUpload getResourcesToUpload() override;

		static VertexInputDescription vertexInputDesc();

		std::shared_ptr<Buffer> vertexIndexBuffer()const
		{
			return _device.mesh_buffer;
		}

		struct CubeMakeInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			Vector3 center = Vector3(0);
			bool face_normal = true;
			bool same_face = true;
		};
		using CMI = CubeMakeInfo;
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
		using SMI = SphereMakeInfo;
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
		using PMI = PlatonMakeInfo;

		static std::shared_ptr<RigidMesh> MakeTetrahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeOctahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeIcosahedron(PlatonMakeInfo const& pmi);
		static std::shared_ptr<RigidMesh> MakeDodecahedron(PlatonMakeInfo const& pmi);
	};
}