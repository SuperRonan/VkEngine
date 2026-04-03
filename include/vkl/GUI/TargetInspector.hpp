#include <vkl/GUI/Panel.hpp>

namespace vkl::GUI
{
	class TargetInspectorBase : public Panel
	{
	protected:

		std::shared_ptr<VkObject> _target;
		bool _enable_source = false;

	public:

		TargetInspectorBase(std::shared_ptr<VkObject> const& target);

		TargetInspectorBase(std::shared_ptr<VkObject> const& target, std::string_view class_type);

		std::shared_ptr<VkObject> const& targetPtr() const
		{
			return _target;
		}

		void setEnableSource(bool value = true)
		{
			_enable_source = true;
		}

		bool enableSource() const
		{
			return _enable_source;
		}

		virtual void declareMenu(Context& ctx) override;

		virtual void declareInline(Context& ctx) override;

		static void DeclareClass(Context& ctx, std::type_info const& type_info);
	};

	template <std::derived_from<VkObject> Target>
	class TargetInspector : public TargetInspectorBase
	{
	public:

		TargetInspector(std::shared_ptr<Target> const& target) :
			TargetInspectorBase(target, typeid(Target).name())
		{

		}

		Target* target() const
		{
			return static_cast<Target*>(_target.get());
		}

		virtual void declareInline(Context& ctx) override
		{
			TargetInspectorBase::declareInline(ctx);
			if (_target)
			{
				DeclareClass(ctx, typeid(Target));
			}
		}
	};
}