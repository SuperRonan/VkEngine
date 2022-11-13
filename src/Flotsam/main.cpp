
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
#include <Core/ImguiCommand.hpp>
#include <Core/Executor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class Flotsam: VkApplication
	{
	protected:

		std::shared_ptr<VkWindow> _main_window = nullptr;


		virtual std::vector<const char* > getDeviceExtensions()override
		{
			std::vector<const char* > res = VkApplication::getDeviceExtensions();

			return res;
		}

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			VkApplication::requestFeatures(features);
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
		}


	public:

		Flotsam(bool validation = false) :
			VkApplication("Flotsam", validation)
		{
			init();
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = "Flotsam",
				.w = 1600,
				.h = 900,
				.resizeable = GLFW_FALSE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);

		}

		virtual ~Flotsam()
		{}

		struct InputState
		{
			bool paused = true;
			bool grabed = false;
			double scroll = 0;
		};

		void processInput(InputState& inputs)
		{
			GLFWwindow* window = _main_window->handle();

			static int prev_pause = glfwGetKey(window, GLFW_KEY_SPACE);


			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			int current_pause = glfwGetKey(window, GLFW_KEY_SPACE);
			if ((current_pause == GLFW_RELEASE) && (prev_pause == GLFW_PRESS))
			{
				inputs.paused = !inputs.paused;
			}
			prev_pause = current_pause;

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
			{
				inputs.grabed = true;
			}
			else
			{
				inputs.grabed = false;
			}
		}

		virtual void run() override
		{
			using namespace std_vector_operators;

			const std::vector<std::string> common_definitions = {

			};

			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "exec",
				.window = _main_window,
				.use_ImGui = false,
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
			exec.declare(render_target_view);

			
			std::shared_ptr<Image> depth_img = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "DepthImg",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_D32_SFLOAT,
				.extent = extend(_main_window->extent()),
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});

			std::shared_ptr<ImageView> depth_view = std::make_shared<ImageView>(ImageView::CI{
				.name = "DepthView",
				.image = depth_img,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.baseMipLevel = 0,
					.levelCount = 1, 
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.create_on_construct = true,
			});
			exec.declare(depth_view);

			std::shared_ptr<Buffer> cube_buffer = std::make_shared<Buffer>(Buffer::CI{
				.app = this,
				.name = "CubeBuffer",
				.size = 1024,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});
			exec.declare(cube_buffer);

			std::shared_ptr<Image> water_surface_img = std::make_shared<Image>(Image::CI{
				.app = this,
				.name = "WaterSurfaceImage",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R32_SFLOAT,
				.extent = VkExtent3D{.width = 16, .height = 16, .depth = 1},
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.create_on_construct = true,
			});

			std::shared_ptr<ImageView> water_surface_view = std::make_shared<ImageView>(ImageView::CreateInfo{
				.name = "WaterSurfaceView",
				.image = water_surface_img,
				.create_on_construct = true,
			});
			exec.declare(water_surface_view);

			std::shared_ptr<Sampler> bilinear_sampler = std::shared_ptr<Sampler>(new Sampler(this, VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.mipLodBias = 0,
				.anisotropyEnable = false,
			}));

			const std::filesystem::path shader_folder = ENGINE_SRC_PATH "/src/Flotsam/";

			std::shared_ptr<ComputeCommand> simul_water = std::make_shared<ComputeCommand>(ComputeCommand::CI{
				.app = this,
				.name = "SimulWater",
				.shader_path = shader_folder / "water.comp",
				.dispatch_size = water_surface_img->extent(),
				.dispatch_threads = true,
				.bindings = {
					Binding{
						.view = water_surface_view,
						.set = 0, .binding = 0,
					},
				},
			});
			exec.declare(simul_water);

			std::shared_ptr<ClearImage> clear_water = std::make_shared<ClearImage>(ClearImage::CI{
				.app = this,
				.name = "ClearWater",
				.view = water_surface_view,
			});
			exec.declare(clear_water);

			std::shared_ptr<VertexCommand> render_water = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "RenderWater",
				.draw_count = (water_surface_img->extent().width - 1) * (water_surface_img->extent().height - 1),
				.bindings = {
					Binding{
						.view = water_surface_view,
						.sampler = bilinear_sampler,
						.set = 0, .binding = 0,
					},
				},
				.color_attachements = { render_target_view},
				.depth_buffer = depth_view,
				.write_depth = false,
				.vertex_shader_path = shader_folder / "water.vert",
				.geometry_shader_path = shader_folder / "water.geom",
				.fragment_shader_path = shader_folder / "water.frag",
				.definitions = common_definitions,
				.blending = Pipeline::BlendAttachementBlendingAlphaDefault(),
			});
			exec.declare(render_water);
			struct RenderWaterPC
			{
				glm::mat4 world2proj;
				uint32_t flags;
			};

			std::shared_ptr<VertexCommand> render_cube = std::make_shared<VertexCommand>(VertexCommand::CI{
				.app = this,
				.name = "RenderCube",
				.draw_count = 6,
				.bindings = {
					Binding{
						.buffer = cube_buffer,
						.set = 0, .binding = 0,
					},
				},
				.color_attachements = { render_target_view},
				.depth_buffer = depth_view,
				.vertex_shader_path = shader_folder / "cube.vert",
				.geometry_shader_path = shader_folder / "cube.geom",
				.fragment_shader_path = shader_folder / "cube.frag",
				.definitions = common_definitions,
				.clear_color = VkClearColorValue{.float32{0, 0, 0, 0}},
				.clear_depth_stencil = VkClearDepthStencilValue{.depth = 1.0,},
			});
			exec.declare(render_cube);
			struct RenderCubePC
			{
				glm::mat4 world2proj;
				uint32_t flags;
			};


			exec.init();

			InputState inputs = {};

			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Direction);

			double t = glfwGetTime(), dt = 0.0;

			size_t update_index = 0;

			glm::vec3 camera_position = glm::vec3(-1.0, 1.0, 1.0);
			glm::vec3 camera_target = glm::vec3(0, 0, 0);
			glm::vec3 camera_direction = glm::normalize(camera_target - camera_position);

			glm::vec3 camera_right = [&]() -> glm::vec3 {
				//glm::mat4 mr = glm::rotate(glm::mat4(1.0), 90.0f, glm::vec3(0, 0, 1));
				//glm::vec4 tmp = mr * glm::vec4(camera_direction, 0.0);
				//return glm::vec3(tmp.x, tmp.y, tmp.z);

				return glm::normalize(glm::cross(camera_direction, glm::vec3(0, 0, -1)));
			} ();

			float camera_distance = 2.0;

			
			exec.beginFrame();
			exec.beginCommandBuffer();

			exec(clear_water);
			
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
				bool should_render = true;

				
				_main_window->pollEvents();
				processInput(inputs);
				if (!inputs.paused)
				{
					should_render = true;
				}
				if (inputs.grabed)
				{
					mouse_handler.update(dt);

					{
						camera_distance *= std::exp(mouse_handler.getScroll() * 0.1);
					}
				}


				if (!inputs.paused || should_render)
				{
					exec.beginFrame();
					exec.beginCommandBuffer();

					if (!inputs.paused)
					{
						++update_index;

					}

					if(should_render) 
					{
						{
							glm::vec3 camera_direction = mouse_handler.direction<float>();
							// TODO express it with a matrix
							std::swap(camera_direction.y, camera_direction.z);
							camera_direction.x = -camera_direction.x;
							const glm::mat4 cam2proj = [&] {glm::mat4 tmp = glm::perspectiveFov<float>(90.0, _main_window->extent().width, _main_window->extent().height, 0.01, 10); tmp[1][1] *= -1; return tmp; }();
							const glm::mat4 world2cam = glm::lookAt(-camera_distance * camera_direction, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
							const glm::mat4 world2proj = cam2proj * world2cam;
							render_cube->setPushConstantsData(RenderCubePC{
								.world2proj = world2proj,
								.flags = static_cast<uint32_t>(update_index % 1),
							});

							render_water->setPushConstantsData(RenderWaterPC{
								.world2proj = world2proj,
								.flags = static_cast<uint32_t>(update_index % 1),
							});
						}

						exec(render_cube);
						exec(render_water);
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
		vkl::Flotsam app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}