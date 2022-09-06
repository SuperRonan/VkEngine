#include <Core/VkApplication.hpp>
#include <Core/VkWindow.hpp>
#include <Core/ImageView.hpp>
#include <Core/Shader.hpp>
#include <Core/Camera2D.hpp>
#include <Core/MouseHandler.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Core/Buffer.hpp>
#include <Core/Sampler.hpp>
#include <Core/Semaphore.hpp>

#include <Core/Executor.hpp>
#include <Core/ComputeCommand.hpp>
#include <Core/TransferCommand.hpp>

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class VkGameOfLife : VkApplication
	{
	protected:

		VkWindow* _main_window;

		VkExtent2D _world_size;

		Sampler _grid_sampler;

		std::vector<Buffer> _mouse_update_buffers;

		std::vector<Semaphore> _render_finished_semaphores;

		void createSampler()
		{
			VkSamplerCreateInfo ci{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_NEAREST,
				//.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_NEAREST,
				//.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.anisotropyEnable = VK_TRUE,
				.maxAnisotropy = 16,
				.maxLod = 1,
				.unnormalizedCoordinates = VK_FALSE,
			};

			_grid_sampler = Sampler(this, ci);
		}

		void fillGrid(std::shared_ptr<ImageView> grid)
		{
			CommandBuffer copy_command(_pools.graphics);
			copy_command.begin();
			std::vector<uint8_t> data(_world_size.width * _world_size.height);
			std::fill(data.begin(), data.end(), 0);
			{
				size_t n_activated = 0.33 * data.size();
				std::mt19937_64 rng;
				for (size_t i = 0; i < n_activated; ++i)
				{
					size_t x = rng() % (_world_size.width / 2) + _world_size.width / 4;
					size_t y = rng() % (_world_size.height / 2) + _world_size.height / 4;
					size_t index = y * _world_size.width + x;
					data[index] = ~data[index];
				}
			}
			
			StagingPool::StagingBuffer staging_buffer;

			grid->recordTransitionLayout(copy_command,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			//staging_buffer = grid->copyToStaging2D(_staging_pool, data.data(), 1);

			//grid->recordSendStagingToDevice2D(copy_command, staging_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			grid->recordTransitionLayout(copy_command, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			
			copy_command.end();
			copy_command.submitAndWait(_queues.graphics);

			
			_staging_pool.releaseStagingBuffer(staging_buffer);

		}


	public:

		VkGameOfLife(bool validation=false) :
			VkApplication("Game of Life", validation)
		{
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.in_flight_size = 2,
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = "Game of Life",
				.w = 2048,
				.h = 1024,
				.resizeable = GLFW_FALSE,
			};
			_main_window = new VkWindow(window_ci);
			//createRenderPass();
			//createFrameBuffers();

			//_world_size = _main_window->extent();
			_world_size = VkExtent2D(_main_window->extent().width / 2, _main_window->extent().height / 2);

		}

		virtual ~VkGameOfLife()
		{
			delete _main_window;
		}

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

			std::shared_ptr<Image> grid_storage_image = std::make_shared<Image>(this, Image::CI{
				.name = "grid_storage_image",
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8_UINT,
				.extent = VkExtent3D{.width = _world_size.width, .height = _world_size.height, .depth = 1},
				.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.create_on_construct = true,
			});

			std::shared_ptr<ImageView> current_grid_view = std::make_shared<ImageView>(this, ImageView::CI {
				.image = grid_storage_image,
				.type = VK_IMAGE_VIEW_TYPE_2D,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.create_on_construct = true,
			});

			std::shared_ptr<ImageView> prev_grid_view = std::make_shared<ImageView>(this, ImageView::CI{
				.image = grid_storage_image,
				.type = VK_IMAGE_VIEW_TYPE_2D,
				.range = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 1,
					.layerCount = 1,
				},
				.create_on_construct = true,
			});

			vkl::Camera2D camera;
			camera.move({ 0.5, 0.5 });
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			const glm::mat3 screen_coords_matrix = vkl::scaleMatrix<3, float>({ 1.0, 1.0 });
			const glm::vec2 move_scale(1.0 / float(_main_window->extent().width), 1.0 / float(_main_window->extent().height));
			glm::mat3 mat_uv_to_grid = screen_coords_matrix * camera.matrix();

			//{
			//	VkWindow::AquireResult aquired = _main_window->aquireNextImage();
			//	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			//	VkSemaphore wait_semaphore = *aquired.semaphore;
			//	VkSemaphore render_finished_semaphore = _render_finished_semaphores[aquired.in_flight_index];
			//	VkSubmitInfo submission{
			//		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			//		.waitSemaphoreCount = 1,
			//		.pWaitSemaphores = &wait_semaphore,
			//		.pWaitDstStageMask = &wait_stage,
			//		.commandBufferCount = 1,
			//		.pCommandBuffers = _commands.data() + aquired.in_flight_index,
			//		.signalSemaphoreCount = 1,
			//		.pSignalSemaphores = &render_finished_semaphore,
			//	};
			//	VkFence submission_fence = *aquired.fence;
			//	vkQueueSubmit(_queues.graphics, 1, &submission, submission_fence);
			//	_main_window->present(1, &render_finished_semaphore);
			//}

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
					glm::vec2 screen_mouse_pos = mouse_handler.currentPosition<float>() * move_scale - glm::vec2(0.5, 0.5);
					camera.zoom(screen_mouse_pos, mouse_handler.getScroll());
					should_render = true;
				}

				mat_uv_to_grid = screen_coords_matrix * camera.matrix();

				if(!paused || should_render)
				{
					//VkWindow::AquireResult aquired = _main_window->aquireNextImage();
					//VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					//if (paused)
					//{
					//	recordCommandBufferRenderOnly(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_uv_to_grid);
					//}
					//else 
					//{
					//	recordCommandBufferUpdateAndRender(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_uv_to_grid);
					//}
					//VkSemaphore render_finished_semaphore = _render_finished_semaphores[aquired.in_flight_index];
					//VkSemaphore wait_semaphore = *aquired.semaphore;
					//VkSubmitInfo submission{
					//	.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					//	.waitSemaphoreCount = 1,
					//	.pWaitSemaphores = &wait_semaphore,
					//	.pWaitDstStageMask = &wait_stage,
					//	.commandBufferCount = 1,
					//	.pCommandBuffers = _commands.data() + aquired.in_flight_index,
					//	.signalSemaphoreCount = 1,
					//	.pSignalSemaphores = &render_finished_semaphore,
					//};
					//VkFence submission_fence = *aquired.fence;
					//vkQueueSubmit(_queues.graphics, 1, &submission, submission_fence);
					//_main_window->present(1, &render_finished_semaphore);
					
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
		vkl::VkGameOfLife app(true);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}