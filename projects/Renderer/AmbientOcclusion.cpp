#include "AmbientOcclusion.hpp"
#include <Core/VkObjects/DetailedVkFormat.hpp>

namespace vkl
{
	AmbientOcclusion::AmbientOcclusion(CreateInfo const& ci) :
		Module(ci.app, ci.name),
		_sets_layouts(ci.sets_layouts),
		_positions(ci.positions),
		_normals(ci.normals),
		_can_rt(ci.can_rt),
		_gui_method(ImGuiListSelection::CI{
			.name = "Method",
			.mode = ImGuiListSelection::Mode::Dropdown,
			.labels = {"SSAO", "RTAO", "RQAO"},
		})
	{
		_gui_method.setIndex(ci.default_method);
		
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;
		_can_rt &= can_as;

		_gui_method.enableOptions(static_cast<uint32_t>(Method::RTAO), _can_rt && can_rt);
		_gui_method.enableOptions(static_cast<uint32_t>(Method::RQAO), _can_rt && can_rq);

		if (_gui_method.options()[_gui_method.index()].disable)
		{
			_gui_method.setIndex(0);
		}

		assert(!!_positions);

		createInternalResources();
	}

	bool AmbientOcclusion::setCanRT(bool want_rt)
	{
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;

		bool res = false;
		if (want_rt != _can_rt)
		{
			_gui_method.enableOptions(static_cast<uint32_t>(Method::RTAO), want_rt && can_rt);
			_gui_method.enableOptions(static_cast<uint32_t>(Method::RQAO), want_rt && can_rq);
			if (_gui_method.options()[_gui_method.index()].disable)
			{
				if (_gui_method.index() == static_cast<uint32_t>(Method::RTAO) && !_gui_method.options()[static_cast<uint32_t>(Method::RQAO)].disable)
				{
					_gui_method.setIndex(static_cast<uint32_t>(Method::RQAO));
				}
				else if (_gui_method.index() == static_cast<uint32_t>(Method::RQAO) && !_gui_method.options()[static_cast<uint32_t>(Method::RTAO)].disable)
				{
					_gui_method.setIndex(static_cast<uint32_t>(Method::RTAO));
				}
				else
				{
					_gui_method.setIndex(static_cast<uint32_t>(Method::SSAO));
				}
			}
			_can_rt = can_rt;
			res = true;
		}
		return res;
	}

	void AmbientOcclusion::createInternalResources()
	{
		const bool can_as = application()->availableFeatures().acceleration_structure_khr.accelerationStructure;
		const bool can_rt = application()->availableFeatures().ray_tracing_pipeline_khr.rayTracingPipeline;
		const bool can_rq = application()->availableFeatures().ray_query_khr.rayQuery;

		_format = VK_FORMAT_R16_SNORM;
		_format_glsl = DetailedVkFormat::Find(_format).getGLSLName();

		Dyn<VkExtent3D> target_extent = [this]()
		{
			VkExtent3D ref = _positions->image()->extent().value();
			return VkExtent3D{
				.width = ref.width / _downscale,
				.height = ref.height / _downscale,
				.depth = ref.depth,
			};
		};

		_target = std::make_shared<ImageView>(Image::CI{
			.app = application(),
			.name = name() + ".target",
			.type = VK_IMAGE_TYPE_2D,
			.format = &_format,
			.extent = target_extent,
			.layers = _positions->image()->layers(),
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_BITS,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
		});

		_sampler = std::make_shared<Sampler>(Sampler::CI{
			.app = application(),
			.name = name() + ".sampler",
			.filter = VK_FILTER_LINEAR,
			.address_mode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		});

		const std::filesystem::path folder = application()->mountingPoints()["ShaderLib"] + "/Rendering/AmbientOcclusion/";
		_method_glsl = "AO_METHOD 0";
		Dyn<DefinitionsList> defs = [this](DefinitionsList & res)
		{
			res.clear();
			res.push_back("OUT_FORMAT "s + _format_glsl);
			res.push_back("AO_SAMPLES "s + std::to_string(_ao_samples));
			res.push_back(_method_glsl);
			return res;
		};

		ShaderBindings bindings = {
			Binding{
				.image = _target,
				.binding = 0,
			},
			Binding{
				.image = _positions,
				.sampler = _sampler,
				.binding = 1,
			},
		};

		if (_normals)
		{
			bindings.push_back(Binding{
				.image = _normals,
				.sampler = _sampler,
				.binding = 2,
			});
		}

		_ssao_compute_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
			.app = application(),
			.name = name() + ".SSAO",
			.shader_path = folder / "AmbientOcclusion.comp",
			.extent = target_extent,
			.dispatch_threads = true,
			.sets_layouts = _sets_layouts,
			.bindings = bindings,
			.definitions = defs,
		});

		std::filesystem::path rtao_shader = folder / "RTAO.glsl";

		if (can_rq)
		{
			_rqao_compute_command = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = application(),
				.name = name() + ".RQAO",
				.shader_path = rtao_shader,
				.extent = target_extent,
				.dispatch_threads = true,
				.sets_layouts = _sets_layouts,
				.bindings = bindings,
				.definitions = defs,
			});
		}


		using RTShader = RayTracingCommand::RTShader;
		if (can_rt)
		{
			_rtao_command = std::make_shared<RayTracingCommand>(RayTracingCommand::CI{
				.app = application(),
				.name = name() + ".RTAO",
				.sets_layouts = _sets_layouts,
				.raygen = RTShader{.path = rtao_shader},
				.misses = {RTShader{.path = rtao_shader}},
				.closest_hits = {RTShader{.path = rtao_shader}},
				.hit_groups = {RayTracingCommand::HitGroup{.closest_hit = 0}},
				.definitions = defs,
				.bindings = bindings,
				.extent = target_extent,
				.max_recursion_depth = 0,
				.create_sbt = true,
			});

			ShaderBindingTable * sbt = _rtao_command->getSBT().get();
			sbt->setRecord(ShaderRecordType::RayGen, 0, 0);
			sbt->setRecord(ShaderRecordType::Miss, 0, 0);
			sbt->setRecord(ShaderRecordType::HitGroup, 0, 0);
		}
	}


	void AmbientOcclusion::updateResources(UpdateContext& ctx)
	{
		_target->updateResource(ctx);
		if (_enable)
		{
			_method_glsl.back() = '0' + _gui_method.index();
			_sampler->updateResources(ctx);

			if (_gui_method.index() == static_cast<uint32_t>(Method::SSAO))
			{
				ctx.resourcesToUpdateLater() += _ssao_compute_command;
			}
			else if (_gui_method.index() == static_cast<uint32_t>(Method::RTAO))
			{
				ctx.resourcesToUpdateLater() += _rtao_command;
			}
			else if (_gui_method.index() == static_cast<uint32_t>(Method::RQAO))
			{
				ctx.resourcesToUpdateLater() += _rqao_compute_command;
			}

			++_seed;
		}
	}

	void AmbientOcclusion::execute(ExecutionRecorder& recorder, const Camera& camera)
	{
		if (_enable)
		{
			//recorder.pushDebugLabel(name());
				
			uint32_t flags = 0;

			if (_normals)
			{
				flags |= 1;
			}

			CommandPC pc{
				.camera_position = camera.position(),
				.flags = flags, 
				.radius = _radius,
				.seed = uint32_t(std::hash<uint32_t>()(_seed)),
			};

			if (_gui_method.index() == static_cast<uint32_t>(Method::SSAO))
			{
				recorder(_ssao_compute_command->with(ComputeCommand::SingleDispatchInfo{
					.pc = pc,
				}));
			}
			else if (_gui_method.index() == static_cast<uint32_t>(Method::RTAO))
			{
				_rtao_command->getSBT()->recordUpdateIFN(recorder);
				recorder(_rtao_command->with(RayTracingCommand::TraceInfo{
					.extent = _target->image()->extent().value(),
					.pc = pc,
				}));
			}
			else if (_gui_method.index() == static_cast<uint32_t>(Method::RQAO))
			{
				recorder(_rqao_compute_command->with(ComputeCommand::SingleDispatchInfo{
					.pc = pc,
				}));
			}

			//recorder.popDebugLabel();
		}
	}

	void AmbientOcclusion::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());

		ImGui::Checkbox("Enable", &_enable);
		_gui_method.declare();
		ImGui::InputInt("Samples", &_ao_samples);
		ImGui::SliderFloat("Radius", &_radius, 0, 0.2);

		if (ImGui::InputInt("Downscale", &_downscale))
		{
			_downscale = std::max(_downscale, 1);
		}

		ImGui::InputInt("Seed", (int*)(&_seed));

		ImGui::PopID();
	}
}