
#include <Core/App/ImGuiApp.hpp>

#include <Core/Rendering/Camera2D.hpp>

#include <Core/IO/MouseHandler.hpp>
#include <Core/IO/ImGuiUtils.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <Core/VkObjects/VkWindow.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Buffer.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/ImguiCommand.hpp>
#include <Core/Execution/LinearExecutor.hpp>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class ParticuleSim : public AppWithWithImGui
	{
	protected:

		VkPresentModeKHR _present_mode = VK_PRESENT_MODE_FIFO_KHR;
		std::shared_ptr<VkWindow> _main_window = nullptr;

		ImGuiContext* _imgui_ctx = nullptr;

		struct Particule
		{
			glm::vec2 position;
			glm::vec2 velocity;
			unsigned int type;
			float radius;
		};

		virtual std::vector<const char* > getDeviceExtensions()override
		{
			std::vector<const char* > res = VkApplication::getDeviceExtensions();
			//res.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
			res.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
			return res;
		}

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			VkApplication::requestFeatures(features);
			features.features_11.storageBuffer16BitAccess = VK_TRUE;
			features.features_11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
			features.features_12.shaderFloat16 = VK_TRUE;
		}


	public:

		ParticuleSim(bool validation = false) :
			AppWithWithImGui(AppWithWithImGui::CI{ .name = PROJECT_NAME, .enable_validation = validation })
		{
			init();
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = &_present_mode,
				.name = PROJECT_NAME,
				.w = 900,
				.h = 900,
				.resizeable = GLFW_TRUE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);

			initImGui(_main_window);
		}

		virtual ~ParticuleSim()
		{}

		void processInput(bool& pause)
		{
			GLFWwindow* window = _main_window->handle();

			static int prev_pause = glfwGetKey(window, GLFW_KEY_SPACE);


			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			int current_pause = glfwGetKey(window, GLFW_KEY_SPACE);
			if ((current_pause == GLFW_RELEASE) && (prev_pause == GLFW_PRESS))
			{
				pause = !pause;
			}

			prev_pause = current_pause;
		}

		virtual void run() override
		{
			using namespace std::containers_operators;

			const uint32_t particule_size = sizeof(Particule);
			uint32_t num_particules_log2 = 10;
			dv_<uint32_t> num_particules = [&num_particules_log2]() {
				uint32_t res = 1;
				for (uint32_t i = 0; i < num_particules_log2; ++i)
					res *= 2;
				return res;
			};
			glm::vec2 world_size(4.0f, 4.0f);
			uint32_t N_TYPES_PARTICULES = 7;
			const VkBool32 use_half_storage = _available_features.features_12.shaderFloat16;
			const uint32_t storage_float_size = use_half_storage ? 2 : 4;
			const uint32_t force_rule_size = 2 * 4 * storage_float_size;
			const uint32_t particule_props_size = 4 * storage_float_size;
			
			dv_<uint32_t> rule_buffer_size = dv_<uint32_t>(&N_TYPES_PARTICULES) * (particule_props_size + N_TYPES_PARTICULES * force_rule_size);
			
			uint32_t seed = 0x2fe75454a5;
			
			dv_<std::vector<std::string>> definitions = [&]() {
				return std::vector<std::string>({
					std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_PARTICULES),
				});
			};
			
			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "exec",
				.window = _main_window,
				.use_ImGui = true,
			});

			exec.getCommonDefinitions().setDefinition("USE_HALF_STORAGE", std::to_string(use_half_storage));

			std::shared_ptr<Buffer> current_particules = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "CurrentParticulesBuffer",
				.size = num_particules * particule_size,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			exec.declare(current_particules);

			std::shared_ptr<Buffer> previous_particules = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "CurrentParticulesBuffer",
				.size = current_particules->size(),
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			exec.declare(previous_particules);

			std::shared_ptr<Buffer> particule_rules_buffer = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "ParticulesRulesBuffer",
			 	.size = rule_buffer_size,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			exec.declare(particule_rules_buffer);

			std::shared_ptr<Image> render_target_img = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "RenderTargetImg",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.extent = _main_window->extent3D(),
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});

			std::shared_ptr<ImageView> render_target_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "RenderTargetView",
				.image = render_target_img,
			});
			exec.declare(render_target_view);
			
			const std::filesystem::path shaders = PROJECT_SRC_PATH;

			std::shared_ptr<ComputeCommand> init_particules = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "InitParticules",
				.shader_path = shaders / "initParticules.comp",
				.dispatch_size = [&]() {return VkExtent3D{.width = *num_particules, .height = 1, .depth = 1}; } ,
				.dispatch_threads = true,
				.bindings = {
					Binding{
						.buffer = current_particules,
						.set = 0,
						.binding = 0,
					},
				},
				.definitions = definitions,
			});
			exec.declare(init_particules);

			struct InitParticulesPC
			{
				uint32_t number_of_particules;
				uint32_t seed;
				glm::vec2 wolrd_size;

			};


			std::shared_ptr<ComputeCommand> init_rules = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "InitCommonRules",
				.shader_path = shaders / "initCommonRules.comp",
				.dispatch_size = [&]() {return VkExtent3D{.width = N_TYPES_PARTICULES, .height = N_TYPES_PARTICULES, .depth = 1}; },
				.dispatch_threads = true,
				.bindings = {
					Binding{
						.buffer = particule_rules_buffer,
						.set = 0,
						.binding = 0,
					},
				},
				.definitions = definitions,
			});
			exec.declare(init_rules);


			std::shared_ptr<ComputeCommand> run_simulation = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "RunSimulation",
				.shader_path = shaders / "update.comp",
				.dispatch_size = init_particules->getDispatchSize(),
				.dispatch_threads = true,
				.bindings = {
					Binding{
						.buffer = previous_particules,
						.set = 0,
						.binding = 0,
					},
					Binding{
						.buffer = current_particules,
						.set = 0,
						.binding = 1,
					},
					Binding{
						.buffer = particule_rules_buffer,
						.set = 0,
						.binding = 2,
					},
				},
				.definitions = definitions,
			});
			exec.declare(run_simulation);

			struct RunSimulationPC
			{
				uint32_t number_of_particules;
				float dt;
				glm::vec2 world_size;
				float friction;
			};

			
			std::shared_ptr<CopyBuffer> copy_to_previous = std::make_shared<CopyBuffer>(CopyBuffer::CI{
				.app = this,
				.name = "CopyToPrevious",
				.src = current_particules,
				.dst = previous_particules,
			});
			exec.declare(copy_to_previous);

			std::shared_ptr<VertexCommand> render = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "Render",
				.draw_count = num_particules,
				.bindings = {
					Binding{
						.buffer = current_particules,
						.set = 0,
						.binding = 0,
					},
					Binding{
						.buffer = particule_rules_buffer,
						.set = 0,
						.binding = 1,
					},

				},
				.color_attachements = {render_target_view},
				.vertex_shader_path = shaders / "render.vert",
				.geometry_shader_path = shaders / "render.geom",
				.fragment_shader_path = shaders / "render.frag",
				.definitions = definitions,
				.clear_color = VkClearColorValue{.int32 = {0, 0, 0, 0}},
			});
			exec.declare(render);

			float friction = 1.0;
			bool reset_particules = true;
			bool reset_rules = true;

			current_particules->addInvalidationCallback({
				.callback = [&]() {
					reset_particules = true;
				},
				.id = nullptr,
			});

			struct RenderPC
			{
				glm::mat4 matrix;
				float zoom;
			};

			

			exec.init();

			ImGuiRadioButtons gui_present_modes({
				"Immediate",
				"Mailbox",
				"Fifo",
				"Fifo relaxed",
			});
			

			bool paused = true;

			vkl::Camera2D camera;
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			DynamicValue<glm::mat3> screen_coords_matrix = [&]() {return vkl::scaleMatrix<3, float>({ 1.0, float(_main_window->extent2D().value().height) / float(_main_window->extent2D().value().width) }); };

			DynamicValue<glm::vec2> move_scale = [&]() {return glm::vec2(2.0 / float(_main_window->extent2D().value().width), 2.0 / float(_main_window->extent2D().value().height)); };

			camera.move(glm::vec2(-1, -1));
			DynamicValue<glm::mat3> mat_world_to_cam = [&]() {return glm::inverse(screen_coords_matrix.value() * camera.matrix()); };


			while (!_main_window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
				}
				bool should_render = false || true;

				_main_window->pollEvents();
				bool p = paused;
				processInput(paused);
				if (!paused)
				{
					should_render = true;
				}

				beginImGuiFrame();
				{
					ImGui::Begin("Control");
					ImGui::SliderFloat("friction", &friction, 0.0, 2.0);
					ImGui::InputInt("log2(Number of particules)", (int*)& num_particules_log2);
					std::string str_n_particules = std::to_string(*num_particules) + " particules";
					ImGui::Text(str_n_particules.c_str());
					bool changed = ImGui::InputInt("Types of particules", (int*)&N_TYPES_PARTICULES);
					reset_rules |= changed;
					reset_particules |= changed;
					ImGui::Checkbox("reset rules", &reset_rules);
					ImGui::Checkbox("reset particules", &reset_particules);

					ImGui::SliderFloat2("World Size", (float*)&world_size, 0.0, 10.0);

					ImGui::Text("Present mode");
					size_t present_mode = gui_present_modes.declare();
					_present_mode = static_cast<VkPresentModeKHR>(present_mode);

					ImGui::End();
				}
				ImGui::EndFrame();

				mouse_handler.update(dt);
				if (mouse_handler.isButtonCurrentlyPressed(GLFW_MOUSE_BUTTON_1))
				{
					camera.move(mouse_handler.deltaPosition<float>() * move_scale.value());
					should_render = true;
				}
				if (mouse_handler.getScroll() != 0)
				{
					//glm::vec3 screen_mouse_pos_tmp = glm::inverse(screen_coords_matrix * camera.matrix()) * glm::vec3(mouse_handler.currentPosition<float>() - glm::vec2(0.5, 0.5), 1.0);
					//glm::vec2 screen_mouse_pos = glm::vec2(screen_mouse_pos_tmp.x, screen_mouse_pos_tmp.y) / screen_mouse_pos_tmp.z;
					glm::vec2 screen_mouse_pos = (mouse_handler.currentPosition<float>() - glm::vec2(_main_window->extent2D().value().width, _main_window->extent2D().value().height) * 0.5f) * move_scale.value();
					camera.zoom(screen_mouse_pos, mouse_handler.getScroll());
					should_render = true;
				}


				{
					exec.updateResources();
					exec.beginFrame();
					exec.beginCommandBuffer();
					
					if (reset_particules)
					{
						exec(init_particules->with({
								.push_constant = InitParticulesPC{
								.number_of_particules = *num_particules,
								.seed = seed,
								.wolrd_size = world_size,
							},
							}));
						seed = std::hash<uint32_t>()(seed);
						reset_particules = false;
					}
					if(reset_rules)
					{
						exec(init_rules->with({ .push_constant = seed, }));

						seed = std::hash<uint32_t>()(seed);
						reset_rules = false;
					}

					if (!paused)
					{
						exec(copy_to_previous);
						exec(run_simulation->with({
							.push_constant = RunSimulationPC{
							.number_of_particules = *num_particules,
							.dt = static_cast<float>(dt),
							.world_size = world_size,
							.friction = friction,
						},
						}));
					}
					
					{
						exec(render->with({
							.pc = RenderPC{
							.matrix = glm::mat4(mat_world_to_cam.value()),
							.zoom = static_cast<float>(mouse_handler.getScroll()),
						},
						}));
					}

					ImGui::Render();
					exec.preparePresentation(render_target_view);
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					exec.endCommandBufferAndSubmit();
					exec.present();

				}
			}
			

			exec.waitForAllCompletion();

			VK_CHECK(vkDeviceWaitIdle(_device), "Failed to wait for completion.");
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
		vkl::ParticuleSim app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}