#include <vkl/Rendering/ImagePicker.hpp>

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

		MyVector<std::string> src_labels(_sources.size());
		for (size_t i = 0; i < _sources.size(); ++i)
		{
			if (_sources[i])
			{
				src_labels[i] = _sources[i]->name();
			}
			else
			{
				src_labels[i] = "No Image";
			}
		}

		_gui_source = ImGuiListSelection::CI{
			.name = "Source",
			.labels = std::move(src_labels),
			.default_index = ci.index,
		};

		_gui_filter = ImGuiListSelection::CI{
			.name = "Filter",
			.options = {
				ImGuiListSelection::Option{
					.name = "Nearest",
				},
				ImGuiListSelection::Option{
					.name = "Linear",
				},
				ImGuiListSelection::Option{
					.name = "Cubic",
					.disable = true,
				},
			},
		};
	}

	void ImagePicker::updateResources(UpdateContext& ctx)
	{
		_dst->updateResource(ctx);

		ctx.resourcesToUpdateLater() += _blitter;
	}

	void ImagePicker::execute(ExecutionRecorder& recorder)
	{
		const size_t index = _gui_source.index();
		assert(index < _sources.size());

		std::shared_ptr<ImageView> const& src = _sources[index];
		
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

	void ImagePicker::declareGUI(GUI::Context& ctx)
	{
		ImGui::PushID(name().c_str());

		if (ImGui::CollapsingHeader(name().c_str()))
		{

			if (_latest_success)
			{
				ImGui::TextColored(ctx.style().valid_green, "Success");
			}
			else
			{
				ImGui::TextColored(ctx.style().invalid_red, "Fail");
			}
			
			_gui_source.declare();

			const char * dst_name = _dst ? _dst->name().c_str() : "No Image";
			ImGui::Text("Destination: %s", dst_name);

			if(_gui_filter.declare())
			{
				switch (_gui_filter.index())
				{
					case 0:
					case 1:
						_filter = static_cast<VkFilter>(_gui_filter.index());
					break;
					case 2:
						_filter = VK_FILTER_CUBIC_EXT;
					break;
				}
			}

		}
		ImGui::PopID();
	}
}