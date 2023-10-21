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
	class Scene : public VkObject, public ResourcesHolder
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

		};
		

		using PerNodeInstanceFunction = std::function<void(std::shared_ptr<Node> const&, Mat4 const& matrix)>;
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

			bool empty() const
			{
				return !_root;
			}
		};

	protected:

		std::shared_ptr<DirectedAcyclicGraph> _tree;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		struct UBO {
			glm::vec3 ambient;
			uint32_t num_lights;
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

		static std::shared_ptr<DescriptorSetLayout> SetLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();

		std::shared_ptr<DescriptorSetAndPool> set();

		virtual ResourcesToDeclare getResourcesToDeclare();

		virtual ResourcesToUpload getResourcesToUpload();

		virtual void notifyDataIsUploaded();

	};
}