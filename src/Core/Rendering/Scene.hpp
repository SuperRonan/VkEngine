#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Rendering/Model.hpp>

#include <Core/Rendering/Light.hpp>

#include <unordered_map>

#include <cassert>

namespace vkl
{
	class Scene : public VkObject
	{
	
	public:

		using Mat4 = glm::mat4;
		using Mat4x3 = glm::mat4x3;

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
		

		using PerNodeInstanceFunction = std::function<bool(std::shared_ptr<Node> const&, Mat4 const& matrix)>;
		using PerNodeAllInstancesFunction = std::function<void(std::shared_ptr<Node> const&, std::vector<Mat4x3> const&)>;
		using PerNodeFunction = std::function<void(std::shared_ptr<Node> const&)>;
		
		class DirectedAcyclicGraph
		{
		protected:
			
			std::shared_ptr<Node> _root = nullptr;

			void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat4 const& matrix, const PerNodeInstanceFunction& f);

			std::unordered_map<std::shared_ptr<Node>, std::vector<Mat4x3>> _flat_dag;

		public:

			DirectedAcyclicGraph(std::shared_ptr<Node> root);

			constexpr const std::shared_ptr<Node>& root() const
			{
				return _root;
			}

			bool checkIsAcyclic() const;

			void flatten();

			void iterateOnDag(const PerNodeInstanceFunction & f);

			void iterateOnFlattenDag(const PerNodeAllInstancesFunction& f);
			void iterateOnFlattenDag(const PerNodeInstanceFunction& f);

			void iterateOnNodes(const PerNodeFunction & f);

			struct NodePath
			{
				std::vector<uint32_t> path;
			};

			struct PositionedNode
			{
				std::shared_ptr<Node> node = nullptr;
				Mat4x3 matrix;
			};

			PositionedNode findNode(NodePath const& path) const;

			bool empty() const
			{
				return !_root;
			}
		};
		using DAG = DirectedAcyclicGraph;

	protected:

		std::shared_ptr<DirectedAcyclicGraph> _tree;

		uint32_t _lights_bindings_base;
		uint32_t _objects_binding_base;
		uint32_t _mesh_bindings_base;
		uint32_t _material_bindings_base;
		uint32_t _textures_bindings_base;
		uint32_t _xforms_bindings_base;

		uint32_t _mesh_capacity = 1024;
		uint32_t _mesh_count = 0;
		uint32_t _texture_2D_capacity = 1024;
		uint32_t _texture_2D_count = 0;


		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		struct MeshData
		{
			size_t unique_index;
		};

		struct TextureData
		{
			size_t unique_index;
		};

		uint32_t _unique_mesh_counter = 0;
		std::unordered_map<std::shared_ptr<Mesh>, MeshData> _meshes;
		uint32_t allocateUniqueMeshID();

		uint32_t _unique_texture_counter = 0;
		std::unordered_map<std::shared_ptr<Texture>, TextureData> _textures;

		struct ModelReference
		{
			uint32_t mesh_id;
			uint32_t material_id;
			uint32_t xform_id;
			uint32_t flags;
		};

		MyVector<Mat4x3> _xforms;
		std::shared_ptr<Buffer> _xforms_buffer;
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

		std::vector<LightGLSL> _lights_glsl;
		std::shared_ptr<Buffer> _lights_buffer;

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
		void prepareForRendering();

		virtual void updateResources(UpdateContext & ctx);

		//static std::shared_ptr<DescriptorSetLayout> SetLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();

		std::shared_ptr<DescriptorSetAndPool> set();

		friend class SceneUserInterface;
	};
}