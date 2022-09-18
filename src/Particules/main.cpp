#include <Core/VkApplication.hpp>
#include <Core/VkWindow.hpp>
#include <Core/ImageView.hpp>
#include <Core/Shader.hpp>
#include <Core/Camera2D.hpp>
#include <Core/MouseHandler.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Core/Buffer.hpp>
#include <Core/Sampler.hpp>
#include <Core/Pipeline.hpp>
#include <Core/PipelineLayout.hpp>
#include <Core/Program.hpp>
#include <Core/Framebuffer.hpp>
#include <Core/DescriptorPool.hpp>
#include <Core/RenderPass.hpp>
#include <Core/Semaphore.hpp>
#include <Core/CommandBuffer.hpp>

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
			return res;
		}

		virtual void requestFeatures(VkPhysicalDeviceFeatures& features) override
		{
			VkApplication::requestFeatures(features);
		}


	public:

		ParticuleSim(bool validation = false) :
			VkApplication("Particules", validation)
		{
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = "Particules",
				.w = 1024,
				.h = 1024,
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
			

			bool paused = true;

			vkl::Camera2D camera;
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			const glm::mat3 screen_coords_matrix = vkl::scaleMatrix<3, float>({ 1.0, float(_main_window->extent().height) / float(_main_window->extent().width)});
			const glm::vec2 move_scale(2.0 / float(_main_window->extent().width), 2.0 / float(_main_window->extent().height));
			glm::mat3 mat_world_to_cam = glm::inverse(screen_coords_matrix * camera.matrix());

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
					if (!paused)
					{

					}
				}

			}

			vkDeviceWaitIdle(_device);
		}
	};

}

int main()
{
	try
	{
		vkl::ParticuleSim app(true);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}