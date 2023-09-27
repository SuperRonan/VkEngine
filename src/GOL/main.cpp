
#include <Core/App/VkApplication.hpp>

#include <Core/VkObjects/VkWindow.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Shader.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/Semaphore.hpp>

#include <Core/Rendering/Camera2D.hpp>
#include <Core/Rendering/DebugRenderer.hpp>

#include <Core/IO/MouseHandler.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Core/Execution/LinearExecutor.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class VkGameOfLife : VkApplication
	{
	protected:

		std::shared_ptr<VkWindow> _main_window;

		VkExtent2D _world_size;

		std::vector<Buffer> _mouse_update_buffers;

		std::vector<Semaphore> _render_finished_semaphores;

	public:

		VkGameOfLife(bool validation=false) :
			VkApplication(PROJECT_NAME, validation)
		{
			init();
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = PROJECT_NAME,
				.w = 1024,
				.h = 512,
				.resizeable = GLFW_TRUE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);
			//createRenderPass();
			//createFrameBuffers();

			//_world_size = _main_window->extent();
			_world_size = VkExtent2D(_main_window->extent2D().value().width * 2, _main_window->extent2D().value().height * 2);

		}

		virtual ~VkGameOfLife()
		{
			
		}

		struct InputState
		{
			bool pause;
			bool reset;
		};

		void processInput(InputState & state)
		{
			GLFWwindow* window = _main_window->handle();

			static int prev_pause = glfwGetKey(window, GLFW_KEY_SPACE);
			static int prev_reset = glfwGetKey(window, GLFW_KEY_ENTER);


			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			int current_pause = glfwGetKey(window, GLFW_KEY_SPACE);
			if ((current_pause == GLFW_RELEASE) && (prev_pause == GLFW_PRESS))
			{
				state.pause = !state.pause;
			}

			int current_reset = glfwGetKey(window, GLFW_KEY_ENTER);
			if ((current_reset == GLFW_RELEASE) && (prev_reset == GLFW_PRESS))
			{
				state.reset = true;
			}


			prev_pause = current_pause;
			prev_reset = current_reset;
		}

		virtual void run() override
		{
			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "Executor",
				.window = _main_window,
				.use_ImGui = false,
				.use_debug_renderer = false,
			});

			MultiDescriptorSetsLayouts sets_layouts;
			sets_layouts += {0, exec.getCommonSetLayout()};

			const VkExtent2D grid_size = _world_size;
			const DynamicValue<VkExtent3D> grid_packed_size = VkExtent3D{
				.width = std::divCeil(grid_size.width, 8u),
				.height = grid_size.height,
				.depth = 1
			};

			std::shared_ptr<Image> grid_storage_image = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "grid_storage_image",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8_UINT,
				.extent = grid_packed_size,
				.layers = 2,
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});

			std::shared_ptr<ImageView> current_grid_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "Current",
				.image = grid_storage_image,
				.type = VK_IMAGE_VIEW_TYPE_2D,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			});
			exec.declare(current_grid_view);

			std::shared_ptr<ImageView> prev_grid_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "Prev",
				.image = grid_storage_image,
				.type = VK_IMAGE_VIEW_TYPE_2D,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 1,
					.layerCount = 1,
				},
			});
			exec.declare(prev_grid_view);

			const std::filesystem::path shaders = PROJECT_SRC_PATH;

			std::shared_ptr<ComputeCommand> init_grid = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "InitGrid",
				.shader_path = shaders / "init_grid.comp",
				.dispatch_size = grid_packed_size,
				.dispatch_threads = true,
				.sets_layouts = sets_layouts,
				.bindings = {
					Binding{
						.view = current_grid_view,
						.binding = 0,
					},
				},
			});
			exec.declare(init_grid);

			std::shared_ptr<ComputeCommand> update_grid = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "UpdateGrid",
				.shader_path = shaders / "update.comp",
				.dispatch_size = grid_packed_size,
				.dispatch_threads = true,
				.sets_layouts = sets_layouts,
				.bindings = {
					Binding{
						.view = prev_grid_view,
						.name = "prev",
					},
					Binding{
						.view = current_grid_view,
						.name = "next",
					},
				},
			});
			exec.declare(update_grid);

			std::shared_ptr<CopyImage> copy_back_to_prev = std::make_shared<CopyImage>(CopyImage::CI{
				.app = this,
				.name = "CopyBackToPrev",
				.src = current_grid_view,
				.dst = prev_grid_view,
			});
			exec.declare(copy_back_to_prev);

			std::shared_ptr<Image> final_image = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "final image",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.extent = _main_window->extent3D(),
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});

			std::shared_ptr<ImageView> final_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "final view",
				.image = final_image,
			});
			exec.declare(final_view);

			//exec.getDebugRenderer()->setTargets(final_view);
			

			std::shared_ptr<ComputeCommand> render_to_final = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "RenderToFinal",
				.shader_path = shaders / "render.comp",
				.dispatch_size = _main_window->extent3D(),
				.dispatch_threads = true,
				.sets_layouts = sets_layouts,
				.bindings = {
					Binding{
						.view = current_grid_view,
						.name = "grid",
					},
					Binding{
						.view = final_view,
						.name = "target",
					}
				},
			});
			exec.declare(render_to_final);

			exec.init();

			vkl::Camera2D camera;
			camera.move({ 0.5, 0.5 });
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			const glm::mat3 screen_coords_matrix = vkl::scaleMatrix<3, float>({ 1.0, 1.0 });
			DynamicValue<glm::vec2> move_scale = [&]() {return glm::vec2(1.0 / float(_main_window->extent2D().value().width), 1.0 / float(_main_window->extent2D().value().height)); };
			glm::mat3 mat_uv_to_grid = screen_coords_matrix * camera.matrix();

			glm::mat4 mat_for_render = mat_uv_to_grid;

			uint32_t seed = 12;

			exec.updateResources();
			exec.beginFrame();
			exec.beginCommandBuffer();
			exec.execute((*init_grid)(ComputeCommand::DI{
				.push_constant = seed
			}));

			exec.execute((* render_to_final)(ComputeCommand::DI{
				.push_constant = mat_for_render,
			}));
			exec.preparePresentation(final_view);
			exec.endCommandBufferAndSubmit();
			exec.present();

			InputState input_state{
				.pause = true,
				.reset = false,
			};

			while (!_main_window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
				}
				bool should_render = true;

				_main_window->pollEvents();
				bool p = input_state.pause;
				processInput(input_state);
				

				mouse_handler.update(dt);
				if (mouse_handler.isButtonCurrentlyPressed(GLFW_MOUSE_BUTTON_1))
				{
					camera.move(mouse_handler.deltaPosition<float>() * move_scale.value());
					should_render = true;
				}
				if (mouse_handler.getScroll() != 0)
				{
					glm::vec2 screen_mouse_pos = mouse_handler.currentPosition<float>() * move_scale.value() - glm::vec2(0.5, 0.5);
					camera.zoom(screen_mouse_pos, mouse_handler.getScroll());
					should_render = true;
				}

				mat_uv_to_grid = screen_coords_matrix * camera.matrix();

				if(!input_state.pause|| should_render)
				{
					
					exec.updateResources();
					exec.beginFrame();
					exec.beginCommandBuffer();

					if (input_state.reset)
					{
						input_state.reset = false;
						seed = std::hash<uint32_t>{}(seed);
						exec.execute((*init_grid)(ComputeCommand::DI{
							.push_constant = seed
						}));
					}

					if (!input_state.pause)
					{
						exec.execute(copy_back_to_prev);
						exec.execute(update_grid);
					}
					mat_for_render = mat_uv_to_grid;
					exec.execute((*render_to_final)(ComputeCommand::DI{
						.push_constant = mat_for_render,
					}));
					exec.preparePresentation(final_view);
					exec.endCommandBufferAndSubmit();
					exec.present();
				}
				
			}

			exec.waitForAllCompletion();

			vkDeviceWaitIdle(_device);
		}
	};

}

int main()
{
	try
	{
		bool vl = true;
#ifdef NDEBUG
		vl = false;
#endif
		vkl::VkGameOfLife app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}