#pragma once

#include <imgui/imgui.h>

#include <set>
#include <memory>

#include "FileDialog.hpp"

namespace vkl
{
	class Sampler;
	class ImageViewInstance;
	class DescriptorSetAndPoolInstance;
	class DescriptorSetLayoutInstance;
}

namespace vkl::GUI
{
	struct Style
	{
		using Color = ImVec4;

		Color valid_green;
		Color invalid_red;
		Color warning_yellow;

		std::vector<Color> stack_colors;
	};

	enum class EnumStyle
	{
		Label = 0,
		Decimal = 1,
		Hexa = 2,
		MAX_VALUE = Hexa,
		Default = Label,
	};

	static inline const char* GetEnumStyleFormat(EnumStyle style)
	{
		const char* fmt = nullptr;
		if (style == EnumStyle::Decimal)
		{
			fmt = "%d";
		}
		else if (style == EnumStyle::Hexa)
		{
			fmt = "0x%x";
		}
		return fmt;
	}

	extern EnumStyle CycleNextEnumStyle(EnumStyle es);

	// p_style may be nullptr
	extern bool DeclareEnumStyleButtonSwitch(EnumStyle* p_style);

	struct TransientPayload
	{
		std::shared_ptr<VkObject> object;
	};

	class Panel;
	class PanelHolder;
	
	class Context
	{
	public:

		template <class T>
		struct BoundResource
		{
			union
			{
				VkDescriptorSet set;
				T handle;
				const void* ptr = nullptr;
			};
		};

		using BoundSampler = BoundResource<VkSampler>;
		using BoundImage = BoundResource<VkImageView>;

		struct ObjectUsedByCommand
		{
			std::shared_ptr<VkObject> object = {};
			const ImGuiViewport* viewport = {}; // nullptr => main viewport, guaranties that the main viewport values are sorted first

			constexpr std::strong_ordering operator<=>(const ImGuiViewport* rhs) const noexcept
			{
				return viewport <=> rhs;
			}

			constexpr std::strong_ordering compareWeak(ObjectUsedByCommand const& rhs) const noexcept
			{
				return *this <=> rhs.viewport;
			}

			constexpr std::strong_ordering compareStrong(ObjectUsedByCommand const& rhs) const noexcept
			{
				std::strong_ordering res = compareWeak(rhs);
				if (res == std::strong_ordering::equal)
				{
					res = object <=> rhs.object;
				}
				return res;
			}

			constexpr std::strong_ordering operator<=>(ObjectUsedByCommand const& rhs) const noexcept
			{
				return compareStrong(rhs);
			}
		};

	protected:

		ImGuiContext * _imgui_context;

		std::shared_ptr<Style> _style;

		uint _stack_counter = 0;
		bool _keep_drag_drop_payload = false;

		std::shared_ptr<FileDialog> _common_file_dialog;

		MyVector<PanelHolder*> _panel_holder_stack;
		MyVector<Panel*> _panel_stack;

		TransientPayload _drag_drop_payload;
		TransientPayload _clipboard_payload;

		std::set<std::shared_ptr<ImageViewInstance>> _frame_images;
		// Sorted by viewport
		MyVector<ObjectUsedByCommand> _objects_to_keep;
		std::shared_ptr<Sampler> _sampler;
		std::shared_ptr<DescriptorSetLayoutInstance> _imgui_texture_set_layout;
		std::shared_ptr<DescriptorSetLayoutInstance> _imgui_sampler_set_layout;

		BoundImage _imgui_bound_image;
		BoundSampler _imgui_bound_sampler;

		EnumStyle _enum_style = EnumStyle::Default;

		size_t _frame_counter = 0;
	public:

		struct CreateInfo
		{
			ImGuiContext * imgui_context = nullptr;
			std::shared_ptr<Style> style = nullptr;
			std::shared_ptr<FileDialog> common_file_dialog = nullptr;
		};
		using CI = CreateInfo;

		Context(CreateInfo const& ci);

		void createInternalResource(std::shared_ptr<DescriptorSetLayoutInstance> const& texture_set_layout, std::shared_ptr<DescriptorSetLayoutInstance> const& sampler_set_layout);

		void begin();

		void end();

		ImGuiContext* getImGuiContext() const
		{
			return _imgui_context;
		}

		Style const& style()const
		{
			assert(_style);
			return *_style;
		}
		
		SDL_Window* getCurrentWindow()
		{
			return static_cast<SDL_Window*>(ImGui::GetWindowViewport()->PlatformHandleRaw);
		}

		const std::shared_ptr<FileDialog>& getCommonFileDialog()
		{
			return _common_file_dialog;
		}

		void pushPanelHolder(PanelHolder* panel);

		void popPanelHolder();

		void pushPanel(Panel* panel);

		void popPanel();

		PanelHolder* getTopPanelHolder(uint index = 0) const;

		PanelHolder* getBottomPanelHolder(uint index = 0) const;

		std::span<PanelHolder* const> getPanelHolderStack() const;

		Panel* getTopPanel(uint index = 0) const;

		Panel* getBottomPanel(uint index = 0) const;

		std::span<Panel* const> getPanelStack() const;

		ImGuiViewport* getCurrentViewportPtrId() const;

		// Must be called during the ImGui declaration
		void keepFrameObjects(std::span<const std::shared_ptr<VkObject>> objs);
		void keepFrameObjectsMove(std::span<std::shared_ptr<VkObject>> objs);
		void keepFrameObject(std::shared_ptr<VkObject> const& obj)
		{
			keepFrameObjects(std::span(&obj, 1));
		}
		void keepFrameObjectMove(std::shared_ptr<VkObject>&& obj)
		{
			keepFrameObjectsMove(std::span(&obj, 1));
		}

		void keepFrameObjects(const ImGuiViewport* vp, std::span<const std::shared_ptr<VkObject>> objs);
		void keepFrameObjectsMove(const ImGuiViewport* vp, std::span<std::shared_ptr<VkObject>> objs);

		// Image that will synchronized to be sampled by ImGui's fragment shader
		void addFrameImage(std::shared_ptr<ImageViewInstance> const& v)
		{
			_frame_images.insert(v);
		}

		std::set<std::shared_ptr<ImageViewInstance>> const& getImages() const
		{
			return _frame_images;
		}

		// Sorted by viewport
		std::span<ObjectUsedByCommand> getMovableFrameObjects()
		{
			std::span<ObjectUsedByCommand> res(_objects_to_keep.data(), _objects_to_keep.size());
			return res;
		}

		MyVector<ObjectUsedByCommand>&& moveFrameObjects()
		{
			return std::move(_objects_to_keep);
		}

		void clearFrameAccumulators()
		{
			_frame_images.clear();
			_objects_to_keep.clear();
		}

		Style::Color pushStack();

		Style::Color popStack();

		TransientPayload& getDragDropPayload()
		{
			return _drag_drop_payload;
		}

		TransientPayload& getClipboardPayload()
		{
			return _clipboard_payload;
		}

		void keepDragDropPayload()
		{
			_keep_drag_drop_payload = true;
		}

		void clearTemporaryData();

		EnumStyle* pEnumStyle()
		{
			return &_enum_style;
		}

		std::shared_ptr<Sampler> const& sampler() const
		{
			return _sampler;
		}

		std::shared_ptr<DescriptorSetLayoutInstance> const& getImGuiTextureSetLayout() const
		{
			return _imgui_texture_set_layout;
		}

		std::shared_ptr<DescriptorSetLayoutInstance> const& getImGuiSamplerSetLayout() const
		{
			return _imgui_sampler_set_layout;
		}

		BoundImage getImGuiBoundImage() const
		{
			return _imgui_bound_image;
		}

		BoundSampler getImGuiBoundSampler() const
		{
			return _imgui_bound_sampler;
		}

		void setImGuiBoundImage(BoundImage img)
		{
			_imgui_bound_image = img;
		}

		void setImGuiBoundSampler(BoundSampler sampler)
		{
			_imgui_bound_sampler = sampler;
		}

		size_t getFrameIndex() const
		{
			return _frame_counter;
		}

		ImGuiKey getButtonShiftKey() const
		{
			return ImGuiKey_ReservedForModShift;
		}
	};

}

static constexpr std::strong_ordering operator<=>(const ImGuiViewport* lhs, vkl::GUI::Context::ObjectUsedByCommand const& rhs) noexcept
{
	return lhs <=> rhs.viewport;
}