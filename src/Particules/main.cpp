
#include <Core/VkApplication.hpp>
#include <Core/Camera2D.hpp>
#include <Core/MouseHandler.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Core/VkWindow.hpp>
#include <Core/ImageView.hpp>
#include <Core/Buffer.hpp>
#include <Core/ComputeCommand.hpp>
#include <Core/GraphicsCommand.hpp>
#include <Core/TransferCommand.hpp>
#include <Core/Executor.hpp>


#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class ParticuleSim : VkApplication
	{
	protected:

		std::shared_ptr<VkWindow> _main_window;

		RenderPass _render_pass;

		size_t _number_of_particules;
		std::vector<Buffer> _state_buffers;
		Buffer _rule_buffer;

		// size: swapchain
		std::vector<Framebuffer> _framebuffers;

		DescriptorPool _update_descriptor_pool, _render_descriptor_pool;
		std::vector<VkDescriptorSet> _update_descriptor_sets, _render_descriptor_sets;

		std::shared_ptr<ComputeProgram> _update_program;
		std::shared_ptr<GraphicsProgram> _render_program;
		Pipeline _update_pipeline, _render_pipeline;

		std::vector<CommandBuffer> _commands;

		std::vector<Semaphore> _render_finished_semaphores;

		std::shared_ptr<DescriptorPool> _imgui_descriptor_pool;

		virtual bool isDeviceSuitable(VkPhysicalDevice const& device) override
		{
			bool res = false;

			VkBool32 present_support = false;
			QueueFamilyIndices indices = findQueueFamilies(device);

			res = indices.graphics_family.has_value();

			if (res)
			{
				//vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.present_family.value(), _surface, &present_support);
				res = vkGetPhysicalDeviceWin32PresentationSupportKHR(device, indices.present_family.value());
			}

			if (res)
			{
				res = checkDeviceExtensionSupport(device);
			}

			//if (res)
			//{
			//	SwapChainSupportDetails swapchain_support_detail = querySwapChainSupport(device);
			//	bool swapchain_adequate = !swapchain_support_detail.formats.empty() && !swapchain_support_detail.present_modes.empty();
			//	res = swapchain_adequate;
			//}

			if (res)
			{
				VkPhysicalDeviceFeatures features;
				vkGetPhysicalDeviceFeatures(device, &features);
				res = features.samplerAnisotropy;
			}

			return res;
		}

		void initImgui()
		{
			const uint32_t N = 1024;
			std::vector<VkDescriptorPoolSize> sizes = {
				{ VK_DESCRIPTOR_TYPE_SAMPLER,					N },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	N },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				N },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				N },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,		N },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,		N },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			N },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			N },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	N },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	N },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,			N },
			};
			_imgui_descriptor_pool = std::make_shared<DescriptorPool>(DescriptorPool(this, VK_NULL_HANDLE));
			VkDescriptorPoolCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
				.maxSets = N,
				.poolSizeCount = (uint32_t)sizes.size(),
				.pPoolSizes = sizes.data(),
			};
			_imgui_descriptor_pool->create(ci);
			
			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(*_main_window, false);

			ImGui_ImplVulkan_InitInfo ii{
				.Instance = _instance,
				.PhysicalDevice = _physical_device,
				.Device = _device,
				.QueueFamily = _queue_family_indices.graphics_family.value(),
				.Queue = _queues.graphics,
				.DescriptorPool = *_imgui_descriptor_pool,
				.MinImageCount = (uint32_t)_main_window->swapchainSize(),
				.ImageCount = (uint32_t)_main_window->swapchainSize(),
				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			};

			//ImGui_ImplVulkan_Init(&init_info, )

		}

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
			res.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
			res.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
			return res;
		}

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			VkApplication::requestFeatures(features);
			features.features_11.storageBuffer16BitAccess = VK_TRUE;
			features.features_12.shaderFloat16 = VK_TRUE;
		}


	public:

		ParticuleSim(bool validation = false) :
			VkApplication("Particules", validation)
		{
			init();
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
				.name = "Particules",
				.w = 2000,
				.h = 1400,
				.resizeable = GLFW_FALSE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);
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
			const uint32_t particule_size = sizeof(Particule);
			const uint32_t num_particules = 1024*4*2;
			const glm::vec2 world_size(4.0f*2, 4.0f*2);
			const uint32_t N_TYPES_PARTICULES = 7;
			const uint32_t use_hlaf_storage = 1;
			const uint32_t storage_float_size = use_hlaf_storage ? 2 : 4;
			const uint32_t force_rule_size = 2 * 4 * storage_float_size;
			const uint32_t particule_props_size = 4 * storage_float_size;
			const uint32_t rule_buffer_size = N_TYPES_PARTICULES * (particule_props_size + N_TYPES_PARTICULES * force_rule_size);
			
			uint32_t seed = 0x2fe7d6d54a5;
			
			std::vector<std::string> definitions = {
				std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_PARTICULES),
				std::string("USE_HALF_STORAGE ") + std::to_string(use_hlaf_storage),
			};

			std::shared_ptr<Buffer> current_particules = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "CurrentParticulesBuffer",
				.size = particule_size * num_particules,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});

			std::shared_ptr<Buffer> previous_particules = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "CurrentParticulesBuffer",
				.size = particule_size * num_particules,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
				});

			std::shared_ptr<Buffer> particule_rules_buffer = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "ParticulesRulesBuffer",
			 	.size = rule_buffer_size,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});

			std::shared_ptr<Image> render_target_img = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "RenderTargetImg",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.extent = extend(_main_window->extent()),
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});

			std::shared_ptr<ImageView> render_target_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "RenderTargetView",
				.image = render_target_img,
				.create_on_construct = true,
			});

			LinearExecutor exec(_main_window);
			
			std::shared_ptr<ComputeCommand> init_particules = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "InitParticules",
				.shader_path = ENGINE_SRC_PATH "/src/Particules/initParticules.comp",
				.dispatch_size = VkExtent3D{.width = num_particules, .height = 1, .depth = 1},
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
				.shader_path = ENGINE_SRC_PATH "/src/Particules/initCommonRules.comp",
				.dispatch_size = VkExtent3D{.width = N_TYPES_PARTICULES, .height = N_TYPES_PARTICULES, .depth = 1},
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
				.shader_path = ENGINE_SRC_PATH "/src/Particules/update.comp",
				.dispatch_size = VkExtent3D{.width = num_particules, .height = 1, .depth = 1},
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
				.vertex_shader_path = ENGINE_SRC_PATH "/src/Particules/render.vert",
				.geometry_shader_path = ENGINE_SRC_PATH "/src/Particules/render.geom",
				.fragment_shader_path = ENGINE_SRC_PATH "/src/Particules/render.frag",
				.definitions = definitions,
			});
			exec.declare(render);


			exec.init();

			bool paused = true;

			vkl::Camera2D camera;
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			const glm::mat3 screen_coords_matrix = vkl::scaleMatrix<3, float>({ 1.0, float(_main_window->extent().height) / float(_main_window->extent().width)});
			const glm::vec2 move_scale(2.0 / float(_main_window->extent().width), 2.0 / float(_main_window->extent().height));
			camera.move(glm::vec2(-1, -1));
			glm::mat3 mat_world_to_cam = glm::inverse(screen_coords_matrix * camera.matrix());

			exec.beginFrame();
			exec.beginCommandBuffer();
			init_particules->setPushConstantsData(InitParticulesPC{
				.number_of_particules = num_particules,
				.seed = seed,
				.wolrd_size = world_size,
			});
			exec.execute(init_particules);
			init_rules->setPushConstantsData(seed);
			exec.execute(init_rules);
			render->setPushConstantsData(glm::mat4(mat_world_to_cam));
			exec.execute(render);
			exec.preparePresentation(render_target_view);
			exec.endCommandBufferAndSubmit();
			exec.present();

			while (!_main_window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
				}
				bool should_render = false;

				_main_window->pollEvents();
				bool p = paused;
				processInput(paused);
				should_render = p != paused;


				mouse_handler.update(dt);
				if (mouse_handler.isButtonCurrentlyPressed(GLFW_MOUSE_BUTTON_1))
				{
					camera.move(mouse_handler.deltaPosition<float>() * move_scale);
					should_render = true;
				}
				if (mouse_handler.getScroll() != 0)
				{
					//glm::vec3 screen_mouse_pos_tmp = glm::inverse(screen_coords_matrix * camera.matrix()) * glm::vec3(mouse_handler.currentPosition<float>() - glm::vec2(0.5, 0.5), 1.0);
					//glm::vec2 screen_mouse_pos = glm::vec2(screen_mouse_pos_tmp.x, screen_mouse_pos_tmp.y) / screen_mouse_pos_tmp.z;
					glm::vec2 screen_mouse_pos = (mouse_handler.currentPosition<float>() - glm::vec2(_main_window->extent().width, _main_window->extent().height) * 0.5f) * move_scale;
					camera.zoom(screen_mouse_pos, mouse_handler.getScroll());
					should_render = true;
				}

				mat_world_to_cam = glm::inverse(screen_coords_matrix * camera.matrix());

				if (!paused || should_render)
				{
					exec.beginFrame();
					exec.beginCommandBuffer();
					
					if (!paused)
					{
						exec.execute(copy_to_previous);
						run_simulation->setPushConstantsData(RunSimulationPC{
							.number_of_particules = num_particules,
							.dt = static_cast<float>(dt),
							.world_size = world_size,
						});
						exec.execute(run_simulation);
					}
					
					{
						render->setPushConstantsData(glm::mat4(mat_world_to_cam));
						exec.execute(render);
					}

					exec.preparePresentation(render_target_view);
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