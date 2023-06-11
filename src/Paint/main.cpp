
#include <Core/App/VkApplication.hpp>

#include <Core/VkObjects/VkWindow.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Shader.hpp>
#include <Core/VkObjects/Buffer.hpp>
#include <Core/VkObjects/Sampler.hpp>
#include <Core/VkObjects/Semaphore.hpp>

#include <Core/Rendering/Camera2D.hpp>

#include <Core/IO/MouseHandler.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <Core/Execution/LinearExecutor.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/GraphicsCommand.hpp>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class Paint : VkApplication
	{
	protected:

		std::shared_ptr<VkWindow> _main_window;

		std::vector<Buffer> _mouse_update_buffers;

		std::vector<Semaphore> _render_finished_semaphores;

	public:

		Paint(bool validation=false) :
			VkApplication(PROJECT_NAME, validation)
		{
			init();
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
				.name = PROJECT_NAME,
				.w = 1024,
				.h = 512,
				.resizeable = GLFW_TRUE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);
		}

		virtual ~Paint()
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
			});

			std::shared_ptr<Image> canvas = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "Canvas",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.extent = _main_window->extent3D(),
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			std::shared_ptr<ImageView> canvas_view = std::make_shared<ImageView>(ImageView::CI{
				.app = this,
				.name = "CanvasView",
				.image = canvas,
			});
			exec.declare(canvas_view);

			const std::filesystem::path shaders = PROJECT_SRC_PATH;

			std::shared_ptr<VertexCommand> render_line = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "RenderLine",
				.draw_count = 1u,
				.color_attachements = {canvas_view},
				.vertex_shader_path = shaders / "render.vert",
				.geometry_shader_path= shaders / "render.geom",
				.fragment_shader_path = shaders / "render.frag",
			});
			struct RenderPC {
				glm::vec2 p0;
				glm::vec2 p1;
				float t;
			};
			exec.declare(render_line);

			exec.init();

			InputState input_state{
				.pause = true,
				.reset = false,
			};

			double t = glfwGetTime(), dt;
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			const auto Convert2 = [&](auto const& v)
			{
				return glm::vec2(v.x, v.y);
			};

			glm::vec2 prev_mouse_pos = Convert2(mouse_handler.currentPosition<float>());
			size_t frame_idx = -1;
			while (!_main_window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
					++frame_idx;
				}
				bool should_render = true;

				_main_window->pollEvents();
				bool p = input_state.pause;
				processInput(input_state);
				
				
				bool render = false;
				mouse_handler.update(dt);
				
				glm::vec2 mouse_pos = prev_mouse_pos;
				if (true || frame_idx % 1024 == 0)
				{
					mouse_pos = Convert2(mouse_handler.currentPosition<float>()) / glm::vec2(_main_window->extent2D().value().width, _main_window->extent2D().value().height) * 2.0f - 1.0f;
				}
				if (mouse_handler.isButtonCurrentlyPressed(GLFW_MOUSE_BUTTON_2))
				{
					render = true;
				}


				if(!input_state.pause|| should_render)
				{
					
					exec.updateResources();
					exec.beginFrame();
					exec.beginCommandBuffer();

					if (render)
					{
						exec(render_line->with({
							.pc = RenderPC{
							.p0 = prev_mouse_pos,
							.p1 = mouse_pos,
							.t = static_cast<float>(t),
						}
							}));
					}
			 		exec.preparePresentation(canvas_view, false);
					exec.endCommandBufferAndSubmit();
					exec.present();
				}
				
				prev_mouse_pos = mouse_pos;
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
		vkl::Paint app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}