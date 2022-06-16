#include <iostream>
#include <Core/VkApplication.hpp>
#include <Core/VkWindow.hpp>
#include <Core/Mesh.hpp>
#include <chrono>

namespace vkl
{

	class Application1 : VkApplication
	{
	protected:

		VkWindow* _main_window;

		VkRenderPass _render_pass;
		std::vector<VkFramebuffer> _framebuffers;

		void createRenderPass()
		{
			VkAttachmentDescription window_color_attach{
				.format = _main_window->format(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			};


			VkAttachmentReference window_color_ref{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			VkSubpassDependency dependency{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			};

			VkSubpassDescription subpass{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &window_color_ref,
			};

			VkRenderPassCreateInfo render_pass_ci{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &window_color_attach,
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = 1,
				.pDependencies = &dependency,
			};
			VK_CHECK(vkCreateRenderPass(_device, &render_pass_ci, nullptr, &_render_pass), "Failed to create a render pass.");
		}

		void createFrameBuffers()
		{
			_framebuffers.resize(_main_window->swapchainSize());
			for (uint32_t i = 0; i < _main_window->swapchainSize(); ++i)
			{
				std::shared_ptr<ImageView> const& attachement = _main_window->view(i);
				std::vector<std::shared_ptr<ImageView>> tmp = { attachement };
				//_framebuffers[i] = Framebuffer(std::move(tmp));
				throw std::runtime_error("Not implemented");
			}
		}

	public:

		Application1() :
			VkApplication("Application 1")
		{
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.in_flight_size = 2,
				.name = "Application 1",
				.w = 800,
				.h = 600,
				.resizeable = GLFW_TRUE,
			};
			_main_window = new VkWindow(window_ci);
			createRenderPass();
			createFrameBuffers();
		}

		virtual ~Application1()
		{
			for (uint32_t i = 0; i < _main_window->swapchainSize(); ++i)
			{
				vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
			}
			vkDestroyRenderPass(_device, _render_pass, nullptr);

			delete _main_window;
		}

		virtual void run() override
		{
			size_t frame_counter = 0;
			static auto start_time = std::chrono::high_resolution_clock::now();

			VkCommandBufferAllocateInfo command_alloc{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = _pools.graphics,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1,
			};
			VkCommandBuffer clear_command;
			vkAllocateCommandBuffers(_device, &command_alloc, &clear_command);

			VkSemaphoreCreateInfo semaphore_ci{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};
			VkSemaphore frame_ready_semaphore;
			vkCreateSemaphore(_device, &semaphore_ci, nullptr, &frame_ready_semaphore);

			vkl::Mesh mesh = vkl::Mesh::Cube(this);
			mesh.createDeviceBuffer();
			mesh.copyToDevice();


			if(false)
			while (!_main_window->shouldClose())
			{
				_main_window->pollEvents();

				auto now = std::chrono::high_resolution_clock::now();
				float t = std::chrono::duration<float, std::chrono::seconds::period>(now - start_time).count();

				VkWindow::AquireResult main_aquired = _main_window->aquireNextImage();

				if (main_aquired.success)
				{
					VkCommandBufferBeginInfo command_begin{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
						.flags = 0,
						.pInheritanceInfo = nullptr,
					};
					vkBeginCommandBuffer(clear_command, &command_begin);

					VkClearValue clear_value{
						.color = VkClearColorValue{1, 0, 1, 1},
					};

					VkRenderPassBeginInfo render_pass_begin{
						.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
						.renderPass = _render_pass,
						.framebuffer = _framebuffers[main_aquired.swap_index],
						.renderArea = VkRect2D{
							.offset = {0, 0},
							.extent = _main_window->extent(),
						},
						.clearValueCount = 1,
						.pClearValues = &clear_value,
					};

					vkCmdBeginRenderPass(clear_command, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdEndRenderPass(clear_command);

					vkEndCommandBuffer(clear_command);

					VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					VkSubmitInfo submission{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.waitSemaphoreCount = 1,
						.pWaitSemaphores = &main_aquired.semaphore,
						.pWaitDstStageMask = &wait_stage,
						.commandBufferCount = 1,
						.pCommandBuffers = &clear_command,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = &frame_ready_semaphore,
					};
					vkQueueSubmit(_queues.graphics, 1, &submission, main_aquired.fence);

					_main_window->present(1, &frame_ready_semaphore);
				}

				++frame_counter;
			}

			vkDeviceWaitIdle(_device);

			vkDestroySemaphore(_device, frame_ready_semaphore, nullptr);
			vkFreeCommandBuffers(_device, _pools.graphics, 1, &clear_command);
		}
	};

}

void main()
{
	try
	{
		vkl::Application1 app;
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}
}