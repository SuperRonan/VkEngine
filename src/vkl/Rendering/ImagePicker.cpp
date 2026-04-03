#include <vkl/Rendering/ImagePicker.hpp>

#include <vkl/GUI/InlinePanel.hpp>
#include <vkl/GUI/ImGuiUtils.hpp>
#include <vkl/GUI/Context.hpp>
#include <vkl/GUI/VulkanEnumWidgets.hpp>

namespace vkl
{
	ImagePicker::ImagePicker(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_sources(ci.sources),
		_dst(ci.dst)
	{

		_blitter = std::make_shared<BlitImage>(BlitImage::CI{
			.app = application(),
			.name = name() + ".Blitter",
		});
	}

	void ImagePicker::updateResources(UpdateContext& ctx)
	{
		_dst->updateResources(ctx);

		ctx.resourcesToUpdateLater() += _blitter;
	}

	void ImagePicker::execute(ExecutionRecorder& recorder)
	{
		if (_source_index < _sources.size())
		{
			std::shared_ptr<ImageView> const& src = _sources[_source_index];
		
			if (src != _dst)
			{
				bool exec = src && _dst && src->instance() && _dst->instance();
				if (exec)
				{
					recorder(_blitter->with(BlitImage::BlitInfo{
						.src = src,
						.dst = _dst,
						.filter = _filter,
					}));
				}
				_latest_success = exec;
			}
		}
		else
		{
			_latest_success = false;
		}
	}

	namespace GUI
	{
		class ImagePickerInspector : public Panel
		{
		protected:
			std::shared_ptr<ImagePicker> _target;
			ImGuiListSelection _source;
			TargetIndirectInlinePanel<ImageView> _current_source, _dst;
		public:
			ImagePickerInspector(std::shared_ptr<ImagePicker> const& target):
				Panel(target->application(), std::format("{}", target->name())),
				_target(target)
			{
				MyVector<std::string> src_labels(_target->_sources.size());
				for (size_t i = 0; i < _target->_sources.size(); ++i)
				{
					if (_target->_sources[i])
					{
						src_labels[i] = _target->_sources[i]->name();
					}
					else
					{
						src_labels[i] = "No Image";
					}
				}
				_source = ImGuiListSelection::CI{
					.name = "Source",
					.labels = std::move(src_labels),
					.default_index = _target->_source_index,
				};

				_current_source.init("Current Source");
				_dst.init("Destination");
			}

			virtual void declareInline(Context& ctx) override
			{
				if (_target->_latest_success)
				{
					ImGui::TextColored(ctx.style().valid_green, "Success");
				}
				else
				{
					ImGui::TextColored(ctx.style().invalid_red, "Fail");
				}

				_source.setIndex(_target->_source_index);
				if (_source.declare())
				{
					_target->_source_index = _source.index();
				}

				const char* dst_name = _target->_dst ? _target->_dst->name().c_str() : "No Image";
				ImGui::Text("Destination: %s", dst_name);

				const bool cubic_allowed = false;
				std::array allowed_filters = { VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_FILTER_CUBIC_EXT };
				InspectVkEnum<VkFilter>(ctx, "Filter", &_target->_filter, std::span(allowed_filters.data(), cubic_allowed ? 3 : 2));

				_current_source.declareInline(ctx, _target->source());
				_dst.declareInline(ctx, _target->_dst);
			}
		};
	}

	std::shared_ptr<GUI::Panel> ImagePicker::makeInspector(std::shared_ptr<VkObject> const& shared_this, GUI::Context& ctx)
	{
		assert(shared_this.get() == this);
		return std::make_shared<GUI::ImagePickerInspector>(std::static_pointer_cast<ImagePicker>(shared_this));
	}
}