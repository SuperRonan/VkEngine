#pragma once

#include <vkl/Rendering/SceneNode.hpp>

namespace vkl
{
	class DirectedAcyclicGraph
	{
	public:
		using Vec3 = Vector3f;
		using Vec4 = Vector4f;
		using Mat3 = Matrix3f;
		using Mat4 = Matrix4f;
		using Mat3x4 = Matrix3x4f;
		using Node = SceneNode;
	private:

		template <class T, std::ranges::range Storage>
		struct NodePathParent : public Storage
		{
			template<class ...Args>
			constexpr NodePathParent(Args&&... args) noexcept :
				Storage(std::forward<Args>(args)...)
			{
			}

			size_t hash() const noexcept
			{
				return HashRange(*this);
			}

			template <class OtherStorage, class RhsPath = NodePathParent<T, OtherStorage>>
			constexpr bool operator==(RhsPath const& rhs) const noexcept
			{
				return std::ranges::equal(*this, rhs);
			}

			template <class OtherStorage, class RhsPath = NodePathParent<T, OtherStorage>>
			constexpr auto operator<=>(RhsPath const& rhs) const noexcept
			{
				return std::ranges::compare_three_way_size_lexicographical(*this, rhs);
			}
		};

	public:

		friend class Scene;

		template <class T>
		struct NodePathViewBase : public NodePathParent<T, std::span<T const>>
		{
			using Parent = NodePathParent<T, std::span<T const>>;
			using Storage = std::span<T const>;

			constexpr NodePathViewBase() noexcept = default;
			constexpr NodePathViewBase(NodePathViewBase const&) noexcept = default;

			template <std::convertible_to<std::span<T const>> OtherStorage>
			constexpr NodePathViewBase(OtherStorage const& s) noexcept :
				Parent(s)
			{
			}

			constexpr NodePathViewBase(T const* ptr, size_t size) noexcept :
				Parent(ptr, size)
			{
			}

			constexpr NodePathViewBase& operator=(NodePathViewBase const&) noexcept = default;

			template <std::convertible_to<std::span<T const>> OtherStorage>
			constexpr NodePathViewBase& operator=(OtherStorage const& rhs) noexcept
			{
				Storage::operator=(rhs);
				return *this;
			}
		};

		template <class T, template <class> class GenContainer = MyVector>
		struct NodePathBase : public NodePathParent<T, GenContainer<T>>
		{
			using Parent = NodePathParent<T, GenContainer<T>>;
			using ViewType = NodePathViewBase<T>;
			using Storage = GenContainer<T>;

			constexpr NodePathBase() = default;
			constexpr NodePathBase(NodePathBase&&) noexcept = default;
			constexpr NodePathBase(NodePathBase const&) = default;

			template <std::ranges::range RhsPath>
				requires std::convertible_to<std::ranges::range_value_t<RhsPath>, T>
			constexpr NodePathBase(RhsPath const& other) :
				Parent(other.begin(), other.end())
			{
			}

			template <std::ranges::range RhsPath>
				requires std::convertible_to<std::ranges::range_value_t<RhsPath>, T>
			constexpr NodePathBase& operator=(RhsPath const& rhs)
			{
				Storage::assign(rhs.begin(), rhs.end());
				return *this;
			}

			constexpr NodePathBase& operator=(NodePathBase&& rhs) noexcept = default;
			constexpr NodePathBase& operator=(NodePathBase const& rhs) = default;

			ViewType view() const noexcept
			{
				return ViewType(*this);
			}

			operator ViewType() const noexcept
			{
				return view();
			}
		};

		using FastNodePathView = NodePathViewBase<uint32_t>;
		using FastNodePath = NodePathBase<uint32_t>;

		using RobustNodePathView = NodePathViewBase<Node*>;
		using RobustNodePath = NodePathBase<Node*>;

		struct PerNodeInstance
		{
			Mat3x4 matrix;
			uint32_t flags;
			Range32u fast_path_ref;
		};

		using PerNodeInstanceFunction = std::function<bool(std::shared_ptr<Node> const&, Mat3x4 const& matrix, uint32_t)>;
		using PerNodeInstanceFastPathFunction = std::function<bool(std::shared_ptr<Node> const&, FastNodePathView, Mat3x4 const&, uint32_t)>;
		using PerNodeInstanceRobustPathFunction = std::function<bool(std::shared_ptr<Node> const&, RobustNodePathView, Mat3x4 const&, uint32_t)>;
		using PerNodeAllInstancesFunction = std::function<void(std::shared_ptr<Node> const&, std::span<const PerNodeInstance>)>;
		using PerNodeFunction = std::function<void(std::shared_ptr<Node> const&)>;


	protected:

		std::shared_ptr<Node> _root = nullptr;

		void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFunction& f);

		void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, FastNodePath& path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceFastPathFunction& f);
		void iterateOnNodeThenSons(std::shared_ptr<Node> const& node, RobustNodePath& path, Mat3x4 const& matrix, uint32_t flags, const PerNodeInstanceRobustPathFunction& f);

		that::ExtensibleStorage<uint32_t> _flat_path_storage = {};
		std::unordered_map<std::shared_ptr<Node>, std::vector<PerNodeInstance>> _flat_dag;

	public:

		DirectedAcyclicGraph(std::shared_ptr<Node> root);

		constexpr const std::shared_ptr<Node>& root() const
		{
			return _root;
		}

		bool checkIsAcyclic() const;

		void flatten();


		void iterateOnDag(const PerNodeInstanceFunction& f);
		void iterateOnDag(const PerNodeInstanceFastPathFunction& f);
		void iterateOnDag(const PerNodeInstanceRobustPathFunction& f);

		void iterateOnFlattenDag(const PerNodeAllInstancesFunction& f);
		void iterateOnFlattenDag(const PerNodeInstanceFunction& f);

		void iterateOnNodes(const PerNodeFunction& f);

		std::span<const PerNodeInstance> getNodeInstancesView(std::shared_ptr<Node> const& node) const;

		struct PositionedNode
		{
			std::shared_ptr<Node> node = nullptr;
			Mat3x4 matrix;
		};

		PositionedNode findNode(FastNodePathView const& path);

		PositionedNode findNode(RobustNodePathView const& path);

		FastNodePath getFastPath(RobustNodePathView const& path);

		RobustNodePath getRobustPath(FastNodePathView const& path);

		FastNodePathView getFlattenFastPath(Range32u ref)
		{
			return FastNodePathView(_flat_path_storage.getSpan(ref));
		}

		bool empty() const
		{
			return !_root;
		}
	};
}