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

#include <iostream>
#include <chrono>
#include <random>

namespace vkl
{

	class VkGameOfLife : VkApplication
	{
	protected:

		VkWindow* _main_window;

		VkRenderPass _render_pass;

		// size: swapchain
		std::vector<Framebuffer> _framebuffers;

		VkExtent2D _world_size;
		 // size: In flight
		std::vector<ImageView> _grids;
	
		VkDescriptorPool _update_descriptor_pool, _render_descriptor_pool;
		std::vector<VkDescriptorSet> _update_descriptor_sets, _render_descriptor_sets;

		Sampler _grid_sampler;

		std::shared_ptr<ComputeProgram> _update_program;
		std::shared_ptr<GraphicsProgram> _render_program;
		Pipeline _update_pipeline, _render_pipeline;

		std::vector<Buffer> _mouse_update_buffers;

		std::vector<VkCommandBuffer> _commands;

		std::vector<VkSemaphore> _render_finished_semaphores;

		void createRenderPass()
		{
			VkAttachmentDescription window_color_attach{
				.format = _main_window->format(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
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

			VkSubpassDependency render_dependency{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			};

			VkSubpassDescription render_subpass{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &window_color_ref,
			};

			VkRenderPassCreateInfo render_pass_ci{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &window_color_attach,
				.subpassCount = 1,
				.pSubpasses = &render_subpass,
				.dependencyCount = 1,
				.pDependencies = &render_dependency,
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
				_framebuffers[i] = Framebuffer(std::move(tmp), _render_pass);
			}
		}

		void createDescriptorPool()
		{
			uint32_t n = _main_window->framesInFlight();

			{
				VkDescriptorPoolSize pool_size{
					.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.descriptorCount = 1,
				};

				// The compute shaders takes two storage imaged (prev and next), so the same pool_size can be used.
				std::array<VkDescriptorPoolSize, 2> pool_sizes{ pool_size, pool_size };

				VkDescriptorPoolCreateInfo ci{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					.maxSets = n,
					.poolSizeCount = pool_sizes.size(),
					.pPoolSizes = pool_sizes.data(),
				};

				VK_CHECK(vkCreateDescriptorPool(_device, &ci, nullptr, &_update_descriptor_pool), "Failed to create a descriptor pool.");
			}
			{
				VkDescriptorPoolSize grid_size{
					.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
				};

				VkDescriptorPoolSize sampler_size{
					.type = VK_DESCRIPTOR_TYPE_SAMPLER,
					.descriptorCount = 1,
				};

				// TODO transform uniform buffer

				std::array<VkDescriptorPoolSize, 2> sizes = { grid_size, sampler_size };

				VkDescriptorPoolCreateInfo ci{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					.maxSets = n,
					.poolSizeCount = sizes.size(),
					.pPoolSizes = sizes.data(),
				};
				VK_CHECK(vkCreateDescriptorPool(_device, &ci, nullptr, &_render_descriptor_pool), "Failed to create a descriptor pool.");
 			}
		}

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

		void createDescriptorSets()
		{
			{
				std::vector<VkDescriptorSetLayout> layouts(_main_window->framesInFlight(), _update_pipeline.program()->setLayouts().front());
				uint32_t n = layouts.size();
				VkDescriptorSetAllocateInfo alloc{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.descriptorPool = _update_descriptor_pool,
					.descriptorSetCount = n,
					.pSetLayouts = layouts.data(),
				};
				_update_descriptor_sets.resize(n);
				VK_CHECK(vkAllocateDescriptorSets(_device, &alloc, _update_descriptor_sets.data()), "Failed to allocate descriptor sets.");
				for (size_t i = 0; i < n; ++i)
				{
					int prev_id = i - 1; if (prev_id < 0)	prev_id = _grids.size() - 1;
					int next_id = i;
					VkDescriptorImageInfo prev{
						.imageView = _grids[prev_id],
						.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					};

					VkDescriptorImageInfo next{
						.imageView = _grids[next_id],
						.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					};

					std::array<VkWriteDescriptorSet, 2> descriptor_writes{
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _update_descriptor_sets[i],
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							.pImageInfo = &prev,
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr,
						},
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _update_descriptor_sets[i],
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							.pImageInfo = &next,
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr,
						},
					};

					vkUpdateDescriptorSets(_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
				}
			}
			{
				uint32_t n = _main_window->framesInFlight();
				std::vector<VkDescriptorSetLayout> layouts(n, _render_pipeline.program()->setLayouts().front());
				VkDescriptorSetAllocateInfo alloc{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.descriptorPool = _render_descriptor_pool,
					.descriptorSetCount = n, // layouts.size(),
					.pSetLayouts = layouts.data(),
				};
				_render_descriptor_sets.resize(n);
				VK_CHECK(vkAllocateDescriptorSets(_device, &alloc, _render_descriptor_sets.data()), "Failed to allocate descriptor sets.");
				for (size_t i = 0; i < n; ++i)
				{
					VkDescriptorImageInfo grid{
						.imageView = _grids[i],
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					};

					VkDescriptorImageInfo sampler{
						.sampler = _grid_sampler.handle(),
					};

					std::array<VkWriteDescriptorSet, 2> descriptor_writes{
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _render_descriptor_sets[i],
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
							.pImageInfo = &grid,
						},
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _render_descriptor_sets[i],
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
							.pImageInfo = &sampler,
						},
					};

					vkUpdateDescriptorSets(_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

				}
			}
		}

		void createComputePipeline()
		{
			std::filesystem::path shader_path = std::string(ENGINE_SRC_PATH) + "/src/GOL/update.comp";

			Shader update_shader(this, shader_path, VK_SHADER_STAGE_COMPUTE_BIT);
			_update_program = std::make_shared<ComputeProgram>(std::move(update_shader));
			_update_pipeline = Pipeline(this, _update_program);
		}

		void createGraphicsPipeline()
		{
			std::filesystem::path vert_shader_path = std::string(ENGINE_SRC_PATH) + "/src/GOL/render.vert";
			std::filesystem::path frag_shader_path = std::string(ENGINE_SRC_PATH) + "/src/GOL/render.frag";

			std::shared_ptr<Shader> vert = std::make_shared<Shader>(Shader(this, vert_shader_path, VK_SHADER_STAGE_VERTEX_BIT));
			std::shared_ptr<Shader> frag = std::make_shared<Shader>(Shader(this, frag_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT));

			_render_program = std::make_shared<GraphicsProgram>(
				GraphicsProgram::CreateInfo{
					._vertex = vert,
					._fragment = frag,
				}
			);

		 
			VkPushConstantRange push_constant{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.offset = 0,
				.size = sizeof(glm::mat4),
			};

			Pipeline::GraphicsCreateInfo ci = {
				._vertex_input = Pipeline::VertexInputWithoutVertices(),
				._input_assembly = Pipeline::InputAssemblyTriangleDefault(),
				._rasterization = Pipeline::RasterizationDefault(),
				._multisampling = Pipeline::MultisampleOneSample(),
				._render_pass = _render_pass,
				._program = _render_program,
			};

			ci.setViewport({ Pipeline::Viewport(_main_window->extent()) }, {Pipeline::Scissor(_main_window->extent())});
			ci.setColorBlending({ Pipeline::BlenAttachementNoBlending() });
			ci.assemble();
			_render_pipeline = Pipeline(this, ci);
		}

		void createGrids()
		{
			_grids.resize(_main_window->framesInFlight());

			Image::CreateInfo grid_image_ci{
				.type = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8_UINT,
				.extent = VkExtent3D{_world_size.width, _world_size.height, 1},
				.use_mips = false,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.elem_size = 1,
			};
			VkCommandBuffer copy_command = beginSingleTimeCommand(_pools.graphics);
			
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
			
			std::vector<StagingPool::StagingBuffer*> staging_buffers(_grids.size());

			for (size_t i = 0; i < _grids.size(); ++i)
			{
				auto& grid = _grids[i];
				grid = ImageView(Image(this, grid_image_ci), VK_IMAGE_ASPECT_COLOR_BIT);

				grid.recordTransitionLayout(copy_command,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

				staging_buffers[i] = grid.copyToStaging2D(_staging_pool, data.data());

				grid.recordSendStagingToDevice2D(copy_command, staging_buffers[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				grid.recordTransitionLayout(copy_command, 
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}

			endSingleTimeCommandAndWait(copy_command, _queues.graphics, _pools.graphics);

			for (auto sb : staging_buffers)
			{
				_staging_pool.releaseStagingBuffer(sb);
			}

		}

		void createCommandBuffers()
		{
			size_t n = _main_window->framesInFlight();
			_commands.resize(n);

			VkCommandBufferAllocateInfo alloc{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = _pools.graphics,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = (uint32_t)_commands.size(),
			};

			VK_CHECK(vkAllocateCommandBuffers(_device, &alloc, _commands.data()), "Failed to allocate command buffers");
		}

		void recordCommandBufferRenderOnly(VkCommandBuffer cmd, size_t grid_id, VkFramebuffer framebuffer, glm::mat4 matrix)
		{
			ImageView& grid = _grids[grid_id];
			vkResetCommandBuffer(cmd, 0);

			VkCommandBufferBeginInfo begin{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};
			vkBeginCommandBuffer(cmd, &begin);
			{
				grid.recordTransitionLayout(cmd,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);

				VkClearValue clear_color{ .color = {0, 0, 0, 1} };

				VkRenderPassBeginInfo render_begin{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = _render_pass,
					.framebuffer = framebuffer,
					.renderArea = VkRect2D{.offset = {0, 0}, .extent = _main_window->extent()},
					.clearValueCount = 1,
					.pClearValues = &clear_color,
				};

				vkCmdBeginRenderPass(cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_pipeline);

					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_program->pipelineLayout(), 0, 1, _render_descriptor_sets.data() + grid_id, 0, nullptr);

					vkCmdPushConstants(cmd, _render_program->pipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrix), glm::value_ptr(matrix));

					vkCmdDraw(cmd, 4, 1, 0, 0);
				}

				vkCmdEndRenderPass(cmd);
				grid.recordTransitionLayout(cmd,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				);
			}
			vkEndCommandBuffer(cmd);
		}

		void recordCommandBufferUpdateAndRender(VkCommandBuffer cmd, size_t grid_id, VkFramebuffer framebuffer, glm::mat4 matrix)
		{
			ImageView& grid = _grids[grid_id];
			
			vkResetCommandBuffer(cmd, 0);

			VkCommandBufferBeginInfo begin{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};

			vkBeginCommandBuffer(cmd, &begin);
			{
				grid.recordTransitionLayout(cmd,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				);
				
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _update_pipeline);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _update_program->pipelineLayout(), 0, 1, &_update_descriptor_sets[grid_id], 0, nullptr);

				const VkExtent3D dispatch_extent = grid.image()->extent();
				const VkExtent3D group_layout = { .width = 16, .height = 16, .depth = 1 };
				vkCmdDispatch(cmd, (dispatch_extent.width + group_layout.width - 1) / group_layout.width, (dispatch_extent.height + group_layout.height - 1) / group_layout.height, (dispatch_extent.depth + group_layout.depth - 1) / group_layout.depth);

				grid.recordTransitionLayout(cmd, 
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);

				VkClearValue clear_color{ .color = {0, 0, 0, 1} };

				VkRenderPassBeginInfo render_begin{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = _render_pass, 
					.framebuffer = framebuffer,
					.renderArea = VkRect2D{.offset = {0, 0}, .extent = _main_window->extent()},
					.clearValueCount = 1,
					.pClearValues = &clear_color,
				};

				vkCmdBeginRenderPass(cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_pipeline);

					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_program->pipelineLayout(), 0, 1, _render_descriptor_sets.data() + grid_id, 0, nullptr);

					vkCmdPushConstants(cmd, _render_program->pipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrix), glm::value_ptr(matrix));

					vkCmdDraw(cmd, 4, 1, 0, 0);
				}

				vkCmdEndRenderPass(cmd);
				grid.recordTransitionLayout(cmd,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				);
			}
			VK_CHECK(vkEndCommandBuffer(cmd), "Failed to record a command buffer.");
			
		}

		void createSemaphores()
		{
			_render_finished_semaphores.resize(_grids.size());

			for (size_t i = 0; i < _render_finished_semaphores.size(); ++i)
			{
				VkSemaphoreCreateInfo ci{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				};
				vkCreateSemaphore(_device, &ci, nullptr, _render_finished_semaphores.data() + i);
			}
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

			createGrids();
			createSampler();

			createRenderPass();
			createFrameBuffers();

			createComputePipeline();
			createGraphicsPipeline();

			createDescriptorPool();
			createDescriptorSets();

			createCommandBuffers();

			createSemaphores();
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

			vkl::Camera2D camera;
			camera.move({ 0.5, 0.5 });
			vkl::MouseHandler mouse_handler(_main_window->handle(), vkl::MouseHandler::Mode::Position);

			double t = glfwGetTime(), dt;

			size_t current_grid_id = 0;

			const glm::mat3 screen_coords_matrix = vkl::scaleMatrix<3, float>({ 1.0, 1.0 });
			const glm::vec2 move_scale(1.0 / float(_main_window->extent().width), 1.0 / float(_main_window->extent().height));
			glm::mat3 mat_uv_to_grid = screen_coords_matrix * camera.matrix();

			{
				VkWindow::AquireResult aquired = _main_window->aquireNextImage();
				VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				recordCommandBufferRenderOnly(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_uv_to_grid);
				VkSubmitInfo submission{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &aquired.semaphore,
					.pWaitDstStageMask = &wait_stage,
					.commandBufferCount = 1,
					.pCommandBuffers = _commands.data() + aquired.in_flight_index,
					.signalSemaphoreCount = 1,
					.pSignalSemaphores = _render_finished_semaphores.data() + aquired.in_flight_index,
				};
				vkQueueSubmit(_queues.graphics, 1, &submission, aquired.fence);
				_main_window->present(1, _render_finished_semaphores.data() + aquired.in_flight_index);
			}

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
					VkWindow::AquireResult aquired = _main_window->aquireNextImage();
					VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					if (paused)
					{
						recordCommandBufferRenderOnly(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_uv_to_grid);
					}
					else 
					{
						recordCommandBufferUpdateAndRender(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_uv_to_grid);
					}
					VkSubmitInfo submission{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.waitSemaphoreCount = 1,
						.pWaitSemaphores = &aquired.semaphore,
						.pWaitDstStageMask = &wait_stage,
						.commandBufferCount = 1,
						.pCommandBuffers = _commands.data() + aquired.in_flight_index,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = _render_finished_semaphores.data() + aquired.in_flight_index,
					};
					vkQueueSubmit(_queues.graphics, 1, &submission, aquired.fence);
					_main_window->present(1, _render_finished_semaphores.data() + aquired.in_flight_index);
					
					if (!paused)
					{
						current_grid_id = (current_grid_id + 1) % _grids.size();
					}
				}
				
			}

			vkDeviceWaitIdle(_device);
		}
	};

}

void main()
{
	try
	{
		vkl::VkGameOfLife app(true);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}
}