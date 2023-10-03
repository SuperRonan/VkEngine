#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Core/App/ImGuiApp.hpp>

#include <Core/VkObjects/VkWindow.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Buffer.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/ImguiCommand.hpp>

#include <Core/Execution/LinearExecutor.hpp>
#include <Core/Execution/Module.hpp>

#include <Core/IO/ImGuiUtils.hpp>
#include <Core/IO/InputListener.hpp>

#include <Core/Rendering/DebugRenderer.hpp>
#include <Core/Rendering/RenderObjects.hpp>
#include <Core/Rendering/Model.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include <thatlib/src/img/ImRead.hpp>

namespace vkl
{
	class Renderer : public Module
	{
	protected:

		Executor& _exec;
		std::shared_ptr<ImageView> _target;
		std::shared_ptr<ImageView> _depth;

	public:

		struct CreateInfo {
			VkApplication* app = nullptr;
			std::string name = {};
			Executor& exec;
			std::shared_ptr<ImageView> target = nullptr;
		};
		using CI = CreateInfo;

		Renderer(CreateInfo const& ci):
			Module(ci.app, ci.name),
			_exec(ci.exec),
			_target(ci.target)
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
		}

		std::shared_ptr<ImageView> depth() const
		{
			return _depth;
		}
	};

	class RendererApp : public AppWithWithImGui
	{
	protected:

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			AppWithWithImGui::requestFeatures(features);
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
		}

	public:

		RendererApp(bool enable_validation):
			AppWithWithImGui(AppWithWithImGui::CI{
				.name = PROJECT_NAME,
				.enable_validation = enable_validation
			})
		{

		}

		virtual void run() final override
		{
			VkApplication::init();

			std::shared_ptr<VkWindow> window = std::make_shared<VkWindow>(VkWindow::CreateInfo{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = PROJECT_NAME,
				.w = 1600,
				.h = 900,
				.resizeable = GLFW_TRUE,
			});
			initImGui(window);

			std::filesystem::path shaders = PROJECT_SRC_PATH;

			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "exec",
				.window = window,
				.use_ImGui = true,
			});

			std::shared_ptr<ImageView> final_image = std::make_shared<ImageView>(ImageView::CI{
				.name = "Final Image View",
				.image_ci = Image::CI{
					.app = this,
					.name = "Final Image",
					.type = VK_IMAGE_TYPE_2D,
					.format = VK_FORMAT_R16G16B16A16_SFLOAT,
					.extent = window->extent3D(),
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
			});
			exec.declare(final_image);
			


			Renderer renderer(Renderer::CI{
				.app = this,
				.name = "renderer",
				.exec = exec,
				.target = final_image,
			});

			std::shared_ptr<RigidMesh> mesh = RigidMesh::MakeOctahedron(RigidMesh::PMI{
				.app = this,
				.radius = 0.5,
				.face_normal = false,
			});

			std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
				.app = this,
				.name = "Model",
				.mesh = mesh,
			});
			exec.declare(model);

			std::shared_ptr<DescriptorSetLayout> model_layout = Model::setLayout(this, Model::SetLayoutOptions{});
			const uint32_t model_set = descriptorBindingGlobalOptions().set_bindings[size_t(DescriptorSetName::object)].set;
			std::shared_ptr<DescriptorSetLayout> common_layout = exec.getCommonSetLayout();
			MultiDescriptorSetsLayouts sets_layouts;
			sets_layouts += {0, common_layout};


			exec.getDebugRenderer()->setTargets(final_image, renderer.depth());

			struct UBO
			{
				float time;
				float delta_time;
				uint32_t frame_idx;
				
				alignas(16) glm::mat4 world_to_camera;
				alignas(16) glm::mat4 camera_to_proj;
				alignas(16) glm::mat4 world_to_proj;
			};
			UBO ubo;
			std::shared_ptr<Buffer> ubo_buffer = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "ubo",
				.size = sizeof(UBO),
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			exec.declare(ubo_buffer);

			std::shared_ptr<UpdateBuffer> update_ubo = std::make_shared<UpdateBuffer>(UpdateBuffer::CI{
				.app = this,
				.name = "UpdateUBO",
				.src = &ubo,
				.dst = ubo_buffer,
			});
			exec.declare(update_ubo);
			

			std::shared_ptr<VertexCommand> render = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "Render",
				.vertex_input_desc = RigidMesh::vertexInputDescStatic(),
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.sets_layouts = (sets_layouts + std::pair{model_set, model_layout}),
				.bindings = {
					Binding{
						.buffer = ubo_buffer,
						.binding = 0,
					},
				},
				.color_attachements = {final_image},
				.depth_buffer = renderer.depth(),
				.write_depth = true,
				.vertex_shader_path = shaders / "render.vert",
				.fragment_shader_path = shaders / "render.frag",
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
			exec.declare(render);
			struct RenderPC {
				glm::mat4 object_to_world;
			};

			std::shared_ptr<VertexCommand> show_3D_basis = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "Show3DGrid",
				.vertex_input_desc = Pipeline::VertexInputWithoutVertices(),
				.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				.draw_count = 3,
				.line_raster_mode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT,
				.sets_layouts = sets_layouts,
				.bindings = {
					Binding{
						.buffer = ubo_buffer,
						.binding = 0,
					}
				},
				.color_attachements = {final_image},
				.vertex_shader_path = shaders / "Show3DBasis.glsl",
				.geometry_shader_path = shaders / "Show3DBasis.glsl",
				.fragment_shader_path = shaders / "Show3DBasis.glsl",
			});
			exec.declare(show_3D_basis);

			
			exec.init();

			struct InputState
			{

			};
			InputState input_state;

			for (int jid = 0; jid <= GLFW_JOYSTICK_LAST; ++jid)
			{
				if (glfwJoystickPresent(jid))
				{
					std::cout << "Joystick " << jid << ": " << glfwGetJoystickName(jid) << std::endl;
				}
			}

			KeyboardListener keyboard(*window);
			MouseListener mouse(*window);
			GamepadListener gamepad(*window, 0);

			const auto process_input = [&](InputState& state)
			{
				if (glfwGetKey(*window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
					glfwSetWindowShouldClose(*window, true);
			};

			double t = glfwGetTime(), dt = 0.0;
			size_t frame_index = 0;

			Camera camera(Camera::CreateInfo{
				.resolution = window->extent2D(),
			});

			FirstPersonCameraController camera_controller(FirstPersonCameraController::CreateInfo{
				.camera = &camera, 
				.keyboard = &keyboard,
				.mouse = &mouse,
				.gamepad = &gamepad,
			});

			while (!window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
				}
				window->pollEvents();

				const auto& imgui_io = ImGui::GetIO();
				
				if(!imgui_io.WantCaptureKeyboard)
				{
					keyboard.update();
				}
				if(!imgui_io.WantCaptureMouse)
				{
					mouse.update();
				}
				
				gamepad.update();
				process_input(input_state);
				camera_controller.updateCamera(dt);

				beginImGuiFrame();
				{
					//ImGui::ShowDemoWindow();

					ImGui::Text("Camera inclination: %f", glm::degrees(camera.inclination()));

					window->declareImGui();

					exec.getDebugRenderer()->declareImGui();
				}
				ImGui::EndFrame();

				ubo.time = static_cast<float>(t);
				ubo.delta_time = static_cast<float>(dt);
				ubo.frame_idx = static_cast<uint32_t>(frame_index);

				ubo.camera_to_proj = camera.getCamToProj();
				ubo.world_to_camera = camera.getWorldToCam();
				ubo.world_to_proj = camera.getWorldToProj();

				{
					exec.updateResources();
					exec.beginFrame();
					exec.beginCommandBuffer();

					if (frame_index == 0)
					{
						UploadResources mesh_uploader(UploadResources::CI{
							.app = this,
						});
						exec(mesh_uploader(UploadResources::UI{
							.holder = mesh,
						}));
					}

					{
						exec(update_ubo);

						exec(render->with(VertexCommand::DrawInfo{
							.drawables = {
								{.drawable = model, .pc = glm::mat4(1)},
							},
						}));

						exec(show_3D_basis);
					}

					exec.renderDebugIFN();
					ImGui::Render();
					exec.preparePresentation(final_image);
					
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					exec.endCommandBufferAndSubmit();
					exec.present();


					++frame_index;
				}
			}

			exec.waitForAllCompletion();

			VK_CHECK(vkDeviceWaitIdle(_device), "Failed to wait for completion.");

		}

	};
}


int main(int argc, char** argv)
{
	try
	{
		bool vl = true;
#ifdef NDEBUG
		vl = false;
#endif
		vkl::RendererApp app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}