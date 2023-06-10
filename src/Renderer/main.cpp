#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Core/ImGuiApp.hpp>

#include <Core/VkWindow.hpp>
#include <Core/ImageView.hpp>
#include <Core/Buffer.hpp>
#include <Core/ComputeCommand.hpp>
#include <Core/GraphicsCommand.hpp>
#include <Core/TransferCommand.hpp>
#include <Core/ImguiCommand.hpp>
#include <Core/LinearExecutor.hpp>
#include <Core/ImGuiUtils.hpp>
#include <Core/Module.hpp>
#include <Core/InputListener.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include "RenderObjects.hpp"
#include "DebugRenderer.hpp"

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
				.target_present_mode = &_present_mode,
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

			DebugRenderer debugger(DebugRenderer::CreateInfo{
				.exec = exec,
				.target = final_image,
			});

			std::shared_ptr<Mesh> mesh = Mesh::MakeSphere(Mesh::SMI{
				.app = this,
				.radius = 1,
			});
			mesh->createDeviceBuffer({_queue_family_indices.graphics_family.value()});
			exec.declare(mesh->combinedBuffer());

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
				.src = ubo,
				.dst = ubo_buffer,
			});
			exec.declare(update_ubo);
			

			std::shared_ptr<VertexCommand> render = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "Render",
				.fetch_vertex_attributes = 3,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.bindings = {
					Binding{
						.buffer = ubo_buffer,
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

			Camera camera;
			float camera_distance = 2.0;

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
				keyboard.update();
				mouse.update();
				gamepad.update();
				process_input(input_state);
				camera_controller.updateCamera(dt);

				beginImGuiFrame();
				{
					ImGui::Text("Camera inclination: %f", glm::degrees(camera.inclination()));

					debugger.declareToImGui();
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
						exec(update_ubo->with(UpdateBuffer::UpdateInfo{
							.src = ObjectView(mesh->vertices().data(), mesh->vertices().size() * sizeof(Vertex)),
							.dst = mesh->combinedBuffer(),
						}));

						exec(update_ubo->with(UpdateBuffer::UpdateInfo{
							.src = ObjectView(mesh->indices().data(), mesh->indices().size() * sizeof(uint32_t)),
							.dst = mesh->combinedBuffer(),
							.offset = mesh->vertices().size() * sizeof(Vertex),
						}));
					}

					{
						exec(update_ubo->with(UpdateBuffer::UpdateInfo{
							.src = ubo,
						}));

						exec(render->with(VertexCommand::DrawInfo{
							.meshes = {
								{.mesh = mesh, .pc = glm::mat4(1)},
							},
						}));

						debugger.execute();
					}


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