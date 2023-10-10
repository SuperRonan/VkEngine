#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Rendering/Model.hpp>

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

			std::vector<std::shared_ptr<Node>> _children = {};

		public:

			struct CreateInfo 
			{
				std::string name = {};
				Mat4x3 matrix = Mat4x3(1);
			};
			using CI = CreateInfo;

			Node(CreateInfo const& ci):
				_name(ci.name),
				_matrix(ci.matrix)
			{}

			constexpr const std::string& name() const
			{
				return _name;
			}

			constexpr const Mat4& matrix4x4() const
			{
				return _matrix;
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

			virtual bool isJustTransform()const
			{
				return !model();
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
		

		using PerNodeFunction = std::function<void(Mat4 const&, Node&)>;

		
		class DirectedAcyclicGraph
		{
		protected:
			
			std::shared_ptr<Node> _root = nullptr;

			void iterateOnNodeThenSons(Node & node, Mat4 const& matrix, const PerNodeFunction& f);

		public:

			DirectedAcyclicGraph(std::shared_ptr<Node> root);

			constexpr const std::shared_ptr<Node>& root() const
			{
				return _root;
			}

			bool checkIsAcyclic() const;

			void iterate(const PerNodeFunction & f);

			bool empty() const
			{
				return !_root;
			}
		};

	protected:

		std::shared_ptr<DirectedAcyclicGraph> _tree;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		void createSet();

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

		static std::shared_ptr<DescriptorSetLayout> SetLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();

		std::shared_ptr<DescriptorSetAndPool> set();

		virtual ResourcesToDeclare getResourcesToDeclare();

		virtual ResourcesToUpload getResourcesToUpload();

		virtual void notifyDataIsUploaded();

	};
}