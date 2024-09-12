#pragma once

#include <vkl/Maths/Types.hpp>

#include <vkl/App/VkApplication.hpp>

#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/AccelerationStructure.hpp>

#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/Execution/GrowableBuffer.hpp>
#include <vkl/Execution/HostManagedBuffer.hpp>

#include <vkl/Rendering/Model.hpp>
#include <vkl/Rendering/Light.hpp>

#include <vkl/Utils/UniqueIndexAllocator.hpp>

#include <vkl/VkObjects/TopLevelAccelerationStructure.hpp>
#include <vkl/Commands/AccelerationStructureCommands.hpp>

#include <unordered_map>

#include <cassert>

namespace vkl
{
	class Scene : public VkObject
	{
	
	public:

		using Mat4 = Matrix4f;
		using Mat4x3 = Matrix4x3f;
		using Mat3x4 = Matrix3x4f;
		using Mat4x3_rm = Matrix4x3fRowMajor;

		class Node
		{
		protected:
			
			std::string _name = {};
			Mat4x3 _matrix = Mat4x3(1);

			bool _visible = true;
			
			std::shared_ptr<Model> _model = nullptr;
			std::shared_ptr<Light> _light = nullptr;

			std::vector<std::shared_ptr<Node>> _children = {};

		public:

			struct CreateInfo 
			{
				std::string name = {};
				Mat4x3 matrix = Mat4x3(1);
				std::shared_ptr<Model> model = nullptr;
			};
			using CI = CreateInfo;

			Node(CreateInfo const& ci):
				_name(ci.name),
				_matrix(ci.matrix),
				_model(ci.model)
			{}

			virtual ~Node() = default;

			constexpr const std::string& name() const
			{
				return _name;
			}

			constexpr Mat4 matrix4x4() const
			{
				return Mat4(_matrix);
			}

			constexpr const Mat4x3& matrix4x3() const
			{
				return _matrix;
			}

			constexpr Mat4x3& matrix4x3()
			{
				return _matrix;
			}

			constexpr void setMatrix(Mat4 const& matrix)
			{
				_matrix = matrix;
			}

			constexpr void setMatrix(Mat4x3 const& matrix)
			{
				_matrix = matrix;
			}

			constexpr bool visible() const
			{
				return _visible;
			}

			void setVisibility(bool v)
			{
				_visible = v;
			}

			constexpr const std::vector<std::shared_ptr<Node>>& children()const
			{
				return _children;
			}

			constexpr const std::shared_ptr<Model>& model()const
			{
				return _model;
			}

			constexpr std::shared_ptr<Model>& model()
			{
				return _model;
			}

			constexpr const std::shared_ptr<Light>& light()const
			{
				return _light;
			}

			constexpr std::shared_ptr<Light>& light()
			{
				return _light;
			}

			void addChild(std::shared_ptr<Node> const& n)
			{
				assert(!!n);
				_children.push_back(n);
			}

			void removeChildIFP(std::shared_ptr<Node> const& n)
			{
				auto it = std::find(_children.begin(), _children.end(), n);
				if (it != _children.end())
				{
					_children.erase(it);
				}
			}

			virtual void updateResources(UpdateContext & ctx);

		};
		
		
		
		class DirectedAcyclicGraph
		{
		public:


			friend class Scene;
			
			struct FastNodePath
			{
				MyVector<uint32_t> path;

				size_t hash() const;

				auto operator<=>(FastNodePath const&) const = default;

				bool operator==(FastNodePath const& other) const = default;
			};

			struct RobustNodePath
			{
				MyVector<Node*> path;
				
				size_t hash() const;

				auto operator<=>(RobustNodePath const&) const = default;

				bool operator==(RobustNodePath const& other) const = default;
			};
			
			struct PerNodeInstance
			{
				Mat4x3 matrix;
				bool visible;
			};

			using PerNodeInstanceFunction = std::function<bool(std::shared_ptr<Node> const&, Mat4 const& matrix)>;
			using PerNodeInstanceFastPathFunction = std::function<bool(std::shared_ptr<Node>, FastNodePath const&, Mat4 const&)>;
			using PerNodeInstanceRobustPathFunction = std::function<bool(std::shared_ptr<Node>, RobustNodePath const&, Mat4 const&, uint32_t)>;
			using PerNodeAllInstancesFunction = std::function<void(std::shared_ptr<Node> const&, std::vector<PerNodeInstance> const&)>;
			using PerNodeFunction = std::function<void(std::shared_ptr<Node> const&)>;


		protected:
			
			std::shared_ptr<Node> _root = nullptr;

			void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat4 const& matrix, const PerNodeInstanceFunction& f);

			void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, FastNodePath & path, Mat4 const& matrix, const PerNodeInstanceFastPathFunction& f);
			void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, RobustNodePath & path, Mat4 const& matrix, uint32_t flags, const PerNodeInstanceRobustPathFunction& f);



			std::unordered_map<std::shared_ptr<Node>, std::vector<PerNodeInstance>> _flat_dag;

		public:

			DirectedAcyclicGraph(std::shared_ptr<Node> root);

			constexpr const std::shared_ptr<Node>& root() const
			{
				return _root;
			}

			bool checkIsAcyclic() const;

			void flatten();


			void iterateOnDag(const PerNodeInstanceFunction & f);
			void iterateOnDag(std::function<bool(std::shared_ptr<Node> const&, FastNodePath const& path, Mat4 const& matrix)> const& f);
			void iterateOnDag(PerNodeInstanceRobustPathFunction const& f);

			void iterateOnFlattenDag(const PerNodeAllInstancesFunction& f);
			void iterateOnFlattenDag(const PerNodeInstanceFunction& f);

			void iterateOnNodes(const PerNodeFunction & f);


			struct PositionedNode
			{
				std::shared_ptr<Node> node = nullptr;
				Mat4x3 matrix;
			};

			PositionedNode findNode(FastNodePath const& path) const;

			PositionedNode findNode(RobustNodePath const& path) const;

			bool empty() const
			{
				return !_root;
			}
		};
		using DAG = DirectedAcyclicGraph;

		struct LightInstanceSpecificData
		{
			virtual ~LightInstanceSpecificData() = default;
		};

	protected:

		std::shared_ptr<DirectedAcyclicGraph> _tree;

		uint32_t _lights_bindings_base;
		uint32_t _objects_binding_base;
		uint32_t _mesh_bindings_base;
		uint32_t _material_bindings_base;
		uint32_t _textures_bindings_base;
		uint32_t _xforms_bindings_base;
		uint32_t _tlas_binding_base;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		struct MeshData
		{
			uint32_t unique_index;
		};

		struct TextureData
		{
			uint32_t unique_index;
		};

		struct MaterialData
		{
			uint32_t unique_index;
		};

		UniqueIndexAllocator _unique_mesh_index_pool;
		std::unordered_map<Mesh*, MeshData> _unique_meshes;

		UniqueIndexAllocator _unique_texture_2D_index_pool;
		std::unordered_map<Texture*, TextureData> _unique_textures;

		UniqueIndexAllocator _unique_material_index_pool;
		std::unordered_map<Material*, MaterialData> _unique_materials;
		std::shared_ptr<HostManagedBuffer> _material_ref_buffer;

		struct MaterialReference
		{
			uint32_t albedo_id;
			uint32_t normal_id;
			uint32_t pad1;
			uint32_t pad2;
		};

		struct ModelReference
		{
			uint32_t mesh_id;
			uint32_t material_id;
			uint32_t xform_id;
			uint32_t flags;
		};
		struct ModelInstance
		{
			uint32_t model_unique_index;
			uint32_t xform_unique_index;
		};
		UniqueIndexAllocator _unique_model_index_pool;
		std::unordered_map<DAG::RobustNodePath, ModelInstance> _unique_models; 
		std::shared_ptr<HostManagedBuffer> _model_references_buffer;

		UniqueIndexAllocator _unique_xform_index_pool;
		std::shared_ptr<HostManagedBuffer> _xforms_buffer;
		std::shared_ptr<Buffer> _prev_xforms_buffer;
		BufferAndRange _xforms_segment;
		BufferAndRange _prev_xforms_segment;

		struct UBO {
			uint32_t num_lights;
			uint32_t num_objects;
			uint32_t num_mesh;
			uint32_t num_materials;
			uint32_t num_textures;
			ubo_vec3 ambient;
		};

		vec3 _ambient = vec3(0.1f);


		std::shared_ptr<Buffer> _ubo_buffer;

		// Per frame number of lights
		uint32_t _num_lights;
		std::shared_ptr<HostManagedBuffer> _lights_buffer;
		UniqueIndexAllocator _unique_light_index_pool;
		UniqueIndexAllocator _light_depth_map_2D_index_pool, _light_depth_map_cube_index_pool;

		
		struct LightInstanceData
		{
			uint32_t unique_id;
			std::shared_ptr<ImageView> depth_view;
			std::unique_ptr<LightInstanceSpecificData> specific_data = nullptr;
			uint32_t depth_texture_unique_id;
			uint32_t frame_light_id;
			uint32_t flags;
		};
		std::unordered_map<DAG::RobustNodePath, LightInstanceData> _unique_light_instances;

		uint32_t _light_resolution = 1024 * 2;
		VkSampleCountFlagBits _light_depth_samples = VK_SAMPLE_COUNT_1_BIT;
		VkFormat _light_depth_format = VK_FORMAT_D32_SFLOAT;



		bool _maintain_rt = false;
		std::shared_ptr<TLAS> _tlas = nullptr;
		std::shared_ptr<BuildAccelerationStructureCommand> _build_tlas;
		BuildAccelerationStructureCommand::BuildInfo _tlas_build_info;
		
		
		UBO getUBO() const;

		void createInternalBuffers();

		void createSet();

		void fillLightsBuffer();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		Scene(CreateInfo const& ci);

		const std::shared_ptr<DirectedAcyclicGraph>& getTree()const
		{
			return _tree;
		}

		const std::shared_ptr<Node>& getRootNode() const
		{
			return _tree->root();
		}

		struct SetLayoutOptions
		{
			
			constexpr bool operator==(const SetLayoutOptions& o) const
			{
				return true;
			}
		};

		// Call before updating resources
		void updateInternal();

		virtual void updateResources(UpdateContext & ctx);

		virtual void prepareForRendering(ExecutionRecorder & exec);

		virtual void buildTLAS(ExecutionRecorder & exec);

		//static std::shared_ptr<DescriptorSetLayout> SetLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();

		std::shared_ptr<DescriptorSetAndPool> set();

		uint32_t objectCount()const
		{
			return _unique_model_index_pool.count();
		}

		VkFormat lightDepthFormat() const
		{
			return _light_depth_format;
		}

		VkSampleCountFlagBits lightDepthSamples() const
		{
			return _light_depth_samples;
		}

		void setMaintainRT(bool maintain_rt);

		friend class SceneUserInterface;
		friend class SimpleRenderer;
	};
}