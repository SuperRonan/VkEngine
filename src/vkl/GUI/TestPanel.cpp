#include <vkl/GUI/TestPanel.hpp>

#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/FancyButtons.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/PanelHolder.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <that/core/Strings.hpp>

#include <vkl/GUI/PopUp.hpp>
#include <vkl/GUI/TargetInspector.hpp>
#include <vkl/GUI/TypedInlineInspector.hpp>
#include <vkl/GUI/InspectorMakeInfo.hpp>

namespace vkl::GUI
{

	class TestObject : public VkObject
	{
	public:

		int value = 0;

		TestObject(VkApplication* app, std::string_view name) :
			VkObject(app, name)
		{

		}

		using InspectorType = class TestObjectInspector;
		friend class InspectorType;
		virtual std::shared_ptr<Panel> makeInspector(GUI::InspectorMakeInfo const& imi) override;
	};

	class TestObjectInspector : public TargetInspector<TestObject>
	{
	public:

		using Parent = TargetInspector<TestObject>;

		TestObjectInspector(std::shared_ptr<TestObject> const& target):
			Parent(target)
		{
			_enable_source = true;
		}

		virtual void declareInline(Context& ctx) override
		{
			Parent::declareInline(ctx);
			ImGui::InputInt("Value", &target()->value);
		}
	};

	

	class TestObjectCreationPanel : public PopUp
	{
	protected:

		std::string _object_name = {};
		int _value = 0;

	public:

		TestObjectCreationPanel(std::string_view name = "Create Test Object"):
			PopUp(name)
		{}

		std::shared_ptr<TestObject> createObject(VkApplication* app)
		{
			std::string_view object_name = _object_name;
			if (object_name.empty())
			{
				object_name = "Test Object";
			}
			auto res = std::make_shared<TestObject>(app, object_name);
			res->value = _value;
			return res;
		}

		virtual Result declareInline(Context& ctx) override
		{
			ImGui::TextFieldEdit("Name", &_object_name, "Test Object");
			ImGui::InputInt("Value", &_value);
			ImGui::Separator();
			auto res = DeclareResultButtons(ctx, _value >= 0);
			return res;
		}
	};

	std::shared_ptr<Panel> TestObject::makeInspector(GUI::InspectorMakeInfo const& imi)
	{
		return std::make_shared<TestObjectInspector>(std::static_pointer_cast<TestObject>(imi.target));
	}

	struct TestPanelInteral
	{
		MyVector<std::shared_ptr<TestObject>> objects = {};
		TestObjectCreationPanel create_object;
		uint create_index = -1;

		MyVector<TypedInlineInspector<TestObject>> inline_panels;

		std::shared_ptr<TestObject> payload;

		Range32i test_bounds = Range32i{.begin = 0, .len = 10};
		Range32i test_range = Range32i{ .begin = 2, .len = 5};
		float test_range_f[2] = {1.12, 5.67};
		float test_f_len_bounds[2] = {0, 1000};
	};

	TestPanelInteral* GetInternal(TestPanel* ptr)
	{
		return reinterpret_cast<TestPanelInteral*>(ptr->getInternal());
	}

	TestPanel::TestPanel(VkApplication* app) :
		Panel(app, "Test")
	{
		TestPanelInteral * internal = new TestPanelInteral;
		_internal = internal;
		TestPanelInteral& i = *internal;
		auto& objects = i.objects;
		auto& panels = i.inline_panels;
		for (int i = 0; i < 3; ++i)
		{
			objects.push_back(std::make_shared<TestObject>(application(), std::format("Test Object {}", i)));
			objects.back()->value = i;
			panels.push_back(TypedInlineInspector<TestObject>(std::format("Object {}", i)));
			auto& b = panels.back();
			b.setEnableSource(true);
			b.setAcceptNullptr(true);
		}
	}

	TestPanel::~TestPanel()
	{
		if (_internal)
		{
			TestPanelInteral* internal = GetInternal(this);
			delete internal;
			_internal = nullptr;
		}
	}

	void TestPanel::declareInline(Context& ctx)
	{
		TestPanelInteral& i = *GetInternal(this);

		{
			int bounds[2] = {i.test_bounds.begin, i.test_bounds.end() - 1};
			if (ImGui::InputInt2("Bounds", bounds))
			{
				i.test_bounds = {.begin = bounds[0], .len = bounds[1] - bounds[0] + 1};
			}
			InspectRange(ctx, "Test Range", &i.test_range, i.test_bounds, true);
			const float range_f_bounds[2] = {static_cast<float>(i.test_bounds.begin), static_cast<float>(i.test_bounds.end())};
			ImGui::SliderRangeEx("Test Float Range", ImGuiDataType_Float, i.test_range_f, range_f_bounds, i.test_f_len_bounds, nullptr, ImGuiSliderFlags_NoRoundToFormat);
			const float len_bounds[2] = {0.0f, range_f_bounds[1]};
			ImGui::SliderRangeEx("Test Float Range Len Bounds", ImGuiDataType_Float, i.test_f_len_bounds, len_bounds, nullptr, nullptr, ImGuiSliderFlags_NoRoundToFormat);
			ImGui::Separator();
		}


		for (size_t j = 0; j < i.inline_panels.size(); ++j)
		{
			ImGui::PushID(j);
			auto& inspector = i.inline_panels[j];
			inspector.setAcceptFunction([](const TestObject* current, const TestObject* incoming, const void* data)
			{
				ObjectInlineInspector::AcceptObjectResult res;
				if (current && incoming)
				{
					res.accept = incoming->value >= current->value;
					if (!res.accept)
					{
						res.reason = "Value must be greater";
					}
				}
				else
				{
					res.accept = true;
				}
				return res;
			});
			std::shared_ptr<VkObject> new_obj = {};
			bool changed = inspector.declareInline(ctx, i.objects[j], &new_obj);
			if (changed)
			{
				if (!new_obj)
				{
					std::string popup_name;
					std::format_to(std::back_inserter(popup_name), "Create Test Object {}###CreateTestObject", j);
					i.create_object.setName(popup_name);
					i.create_object.open(ctx);
					i.create_index = j;
				}
				else
				{
					i.objects[j] = std::dynamic_pointer_cast<TestObject>(new_obj);
				}
			}
			if (i.create_index == j)
			{
				auto creation_res = i.create_object.declare(ctx);
				if (creation_res == PopUp::Result::Create)
				{
					i.objects[i.create_index] = i.create_object.createObject(application());
				}
				if (creation_res != PopUp::Result::Pending)
				{
					i.create_index = uint(-1);
				}
			}
			ImGui::PopID();
		}

		{
			{
				float fw = ImGui::GetItemRect().GetWidth();
				float bw = ImGui::GetDefaultBoxSize(false).x;
				float w = ImMax(ImFloor(std::lerp(bw, fw, 0.25f)), bw);
				ImGui::SetNextItemWidth(w);
				ImGui::CenterNextItem(w);
			}
			if (ImGui::PlusButton("Add new slot"))
			{
				i.objects.push_back(nullptr);
				i.inline_panels.push_back(TypedInlineInspector<TestObject>(std::format("Object {}", i.inline_panels.size32() - 1)));
				auto& b = i.inline_panels.back();
				b.setEnableSource(true);
				b.setAcceptNullptr(true);
			}
		}
	}
}