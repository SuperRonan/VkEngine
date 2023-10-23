#include "Renderer.hpp"

namespace vkl
{
	SimpleRenderer::SimpleRenderer(CreateInfo const& ci):
		Module(ci.app, ci.name),
		_exec(ci.exec),
		_sets_layouts(ci.sets_layouts),
		_scene(ci.scene),
		_target(ci.target)
	{
		createInternalResources();
	}

	void SimpleRenderer::createInternalResources()
	{
		_depth = std::make_shared<ImageView>(ImageView::CI{
			.app = application(),
			.name = name() + ".depth",
			.image_ci = Image::CI{
				.app = application(),
				.name = name() + ".depthImage",
				.type = _target->image()->type(),
				.format = VK_FORMAT_D32_SFLOAT,
				.extent = _target->image()->extent(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			},
			.range = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			});
		_exec.declare(_depth);


		_ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
			.app = application(),
			.name = name() + ".ubo",
			.size = sizeof(UBO),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
		_exec.declare(_ubo_buffer);


		const uint32_t model_set = application()->descriptorBindingGlobalOptions().set_bindings[size_t(DescriptorSetName::object)].set;
		std::shared_ptr<DescriptorSetLayout> model_layout = Model::setLayout(application(), Model::SetLayoutOptions{});

		std::filesystem::path shaders = PROJECT_SRC_PATH;


		_render_scene_direct = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".RenderSceneDirect",
			.vertex_input_desc = RigidMesh::vertexInputDescFullVertex(),
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.sets_layouts = (_sets_layouts + std::pair{model_set, model_layout}),
			.bindings = {
				Binding{
					.buffer = _ubo_buffer,
					.binding = 0,
				},
			},
			.color_attachements = {_target},
			.depth_buffer = _depth,
			.write_depth = true,
			.vertex_shader_path = shaders / "render.vert",
			.fragment_shader_path = shaders / "render.frag",
			.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
			.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
		_exec.declare(_render_scene_direct);


		_render_3D_basis = std::make_shared<VertexCommand>(VertexCommand::CI{
			.app = application(),
			.name = name() + ".Show3DGrid",
			.vertex_input_desc = Pipeline::VertexInputWithoutVertices(),
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.draw_count = 3,
			.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
			.sets_layouts = _sets_layouts,
			.bindings = {
				Binding{
					.buffer = _ubo_buffer,
					.binding = 0,
				}
			},
			.color_attachements = {_target},
			.vertex_shader_path = shaders / "Show3DBasis.glsl",
			.geometry_shader_path = shaders / "Show3DBasis.glsl",
			.fragment_shader_path = shaders / "Show3DBasis.glsl",
			});
		_exec.declare(_render_3D_basis);

		_update_buffer = std::make_shared<UpdateBuffer>(UpdateBuffer::CI{
				.app = application(),
				.name = "UpdateUBO",
			});
		_exec.declare(_update_buffer);
	}

	std::vector<VertexCommand::DrawModelInfo> SimpleRenderer::generateVertexDrawList()
	{
		std::vector<VertexCommand::DrawModelInfo> res;

		auto add_model = [&res](std::shared_ptr<Scene::Node> const& node, glm::mat4 const& matrix)
		{
			if (node->visible() && node->model())
			{
				res.push_back(VertexCommand::DrawModelInfo{
					.drawable = node->model(),
					.pc = matrix,
					});
			}
			return node->visible();
		};

		_scene->getTree()->iterateOnDag(add_model);
		return res;
	}

	void SimpleRenderer::execute(Camera const& camera, float time, float dt, uint32_t frame_id)
	{
		UBO ubo{
			.time = time,
			.delta_time = dt,
			.frame_idx = frame_id,
			
			.world_to_camera = camera.getWorldToCam(),
			.camera_to_proj = camera.getCamToProj(),
			.world_to_proj = camera.getWorldToProj(),
		};

		_exec(_update_buffer->with(UpdateBuffer::UpdateInfo{
			.src = ubo,
			.dst = _ubo_buffer,
		}));

		std::vector<VertexCommand::DrawModelInfo> draw_list = generateVertexDrawList();

		_exec(_render_scene_direct->with(VertexCommand::DrawInfo{
			.drawables = draw_list,
		}));

		_exec(_render_3D_basis);
	}
}