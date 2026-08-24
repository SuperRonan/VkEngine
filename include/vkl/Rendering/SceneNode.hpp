#pragma once

#include <vkl/Core/VulkanCommons.hpp>
#include <vkl/App/VkApplication.hpp>
#include <vkl/Maths/Types.hpp>
#include <vkl/Maths/AffineXForm.hpp>

#include <vkl/Rendering/Model.hpp>
#include <vkl/Rendering/Light.hpp>


namespace vkl
{
	class Scene;

	class SceneNode : public VkObject
	{
	public:
		using Vec3 = Vector3f;
		using Vec4 = Vector4f;
		using Mat3 = Matrix3f;
		using Mat4 = Matrix4f;
		using Mat3x4 = Matrix3x4f;
	protected:

		Mat3x4 _matrix = Mat3x4::Identity();

		Vec3 _translation = Vec3::Zero();
		Vec3 _scale = Vec3::Ones();
		Vec3 _rotation = Vec3::Zero();

		AABB3f _aabb = {};

		bool _visible = true;
		size_t _latest_update_tick = size_t(0);

		std::shared_ptr<Model> _model = nullptr;
		std::shared_ptr<Light> _light = nullptr;

		std::vector<std::shared_ptr<SceneNode>> _children = {};

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			Mat3x4 matrix = Mat3x4::Identity();
			std::shared_ptr<Model> model = nullptr;
		};
		using CI = CreateInfo;

		SceneNode(CreateInfo const& ci);

		virtual ~SceneNode() = default;

		constexpr void setMatrix(Mat3x4 const& matrix)
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

		constexpr const std::span<const std::shared_ptr<SceneNode>> children()const
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

		void addChild(std::shared_ptr<SceneNode> const& n)
		{
			assert(!!n);
			_children.push_back(n);
		}

		void removeAllChildren();

		void removeChild(uint32_t index);

		bool removeChildIFP(const SceneNode* n);

		Mat3x4 getAuxiliaryTransform() const
		{
			Mat3 R = Rotation3D(_rotation);
			Mat3 S = DiagonalMatrixV(_scale);
			Mat3 Q = R * S;
			Mat3x4 res = MakeAffineTransform(Q, _translation);
			return res;
		}

		void resetAuxiliaryTransform()
		{
			_rotation = Vector3f::Zero();
			_scale = Vector3f::Ones();
			_translation = Vector3f::Zero();
		}

		Mat3x4 matrix3x4() const
		{
			Mat3x4 aux = getAuxiliaryTransform();
			Mat3x4 res = _matrix * aux;
			return res;
		}

		Mat4 matrix4x4() const
		{
			return Mat4(matrix3x4());
		}

		Mat3x4 getXForm() const
		{
			return matrix3x4();
		}

		void collapseAuxiliaryTransform();

		virtual void updateResources(UpdateContext& ctx);

		Vec3& scale()
		{
			return _scale;
		}

		Vec3& rotation()
		{
			return _rotation;
		}

		Vec3& translation()
		{
			return _translation;
		}

		const Vec3& scale()const
		{
			return _scale;
		}

		const Vec3& rotation()const
		{
			return _rotation;
		}

		const Vec3& translation()const
		{
			return _translation;
		}

		const AABB3f& getAABB() const
		{
			return _aabb;
		}

		virtual std::shared_ptr<GUI::Panel> makeInspector(GUI::InspectorMakeInfo const& imi) override;
	};
}