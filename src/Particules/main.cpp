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

		std::vector<VkCommandBuffer> _commands;

		std::vector<Semaphore> _render_finished_semaphores;

		struct Particule
		{
			glm::vec2 position;
			glm::vec2 velocity;
			unsigned int type;
			float radius;
		};

#define N_TYPES_OF_PARTICULES 9

		struct ParticuleCommonProperties
		{
			glm::vec4 color;
		};

		struct ForceDescription
		{
			glm::vec4 intensity_inv_linear_inv_linear2_contant_linear;
			glm::vec4 intensity_gauss_mu_sigma;
		};

		void createCommonRuleBuffer()
		{
			struct CommonRuleBuffer
			{
				ParticuleCommonProperties particules_properties[N_TYPES_OF_PARTICULES];
				ForceDescription force_descriptions[N_TYPES_OF_PARTICULES * N_TYPES_OF_PARTICULES];
			};

			const std::uniform_real_distribution<float> u01(0, 1);
			const std::uniform_real_distribution<float> um11(-1, 1);
			const size_t seed = 0x1282357458;
			std::mt19937_64 rng(seed);

			CommonRuleBuffer common_buffer;

			for (size_t type = 0; type < N_TYPES_OF_PARTICULES; ++type)
			{
				common_buffer.particules_properties[type] = ParticuleCommonProperties{
					.color = glm::vec4(u01(rng), u01(rng), u01(rng), 1),
				};

				for (size_t other = 0; other < N_TYPES_OF_PARTICULES; ++other)
				{
					
					const float f1 = 0, f2 = 0, f3 = 0.0, f4 = 0;

					const float g = (type == other ? u01(rng) : um11(rng)) * 2;
					const float mu = (u01(rng) + 1) * 0.1;
					const float sigma = (u01(rng) + 1) * 0.00 + 0.05;
					
					common_buffer.force_descriptions[type * N_TYPES_OF_PARTICULES + other] = ForceDescription{
						.intensity_inv_linear_inv_linear2_contant_linear = glm::vec4(f1, f2, f3, f4),
						.intensity_gauss_mu_sigma = glm::vec4(g, mu, sigma, 0),
					};
					
				}
			}

			_rule_buffer = Buffer(this);
			_rule_buffer.createBuffer(sizeof(CommonRuleBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			VkCommandBuffer cmd = beginSingleTimeCommand(_pools.graphics);
			auto* sb = _rule_buffer.copyToStaging(&common_buffer, sizeof(common_buffer));
			_rule_buffer.recordCopyStagingToBuffer(cmd, sb);
			endSingleTimeCommandAndWait(cmd, _queues.graphics, _pools.graphics);
			_staging_pool.releaseStagingBuffer(sb);
		}

		void createStateBuffers()
		{
			createCommonRuleBuffer();
			_number_of_particules = 128*32;
			std::vector<Particule> particules(_number_of_particules);

			const size_t seed = 0;
			std::mt19937_64 rng(seed);
			const float radius = 3.0;
			std::uniform_real_distribution<float> distrib(0, 2.0*radius);

			for (size_t i = 0; i < _number_of_particules; ++i)
			{
				const float x = distrib(rng);
				const float y = distrib(rng);
				particules[i].position = glm::vec2(x, y);
				particules[i].velocity = glm::vec2(0, 0);
				particules[i].type = rng() % N_TYPES_OF_PARTICULES;
				particules[i].radius = (distrib(rng) * 0.00 + 0.02);
			}

			const VkDeviceSize buffer_size = particules.size() * sizeof(Particule);

			VkCommandBuffer copy_command = beginSingleTimeCommand(_pools.graphics);

			_state_buffers.resize(2);
			std::vector<StagingPool::StagingBuffer*> staging_buffers(_state_buffers.size());
			for (size_t i = 0; i < _state_buffers.size(); ++i)
			{
				_state_buffers[i] = Buffer(this);
				_state_buffers[i].createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
				staging_buffers[i] = _state_buffers[i].copyToStaging(particules.data(), buffer_size);
				_state_buffers[i].recordCopyStagingToBuffer(copy_command, staging_buffers[i]);
			}

			endSingleTimeCommandAndWait(copy_command, _queues.graphics, _pools.graphics);

			for (auto* sb : staging_buffers)
			{
				_staging_pool.releaseStagingBuffer(sb);
			}
		}

		virtual std::vector<const char* > getDeviceExtensions()const override
		{
			std::vector<const char* > res = VkApplication::getDeviceExtensions();
			
			return res;
		}

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

			_render_pass = RenderPass(
				this,
				{ window_color_attach },
				{ {window_color_ref} },
				{ render_subpass },
				{ render_dependency }
			);
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


			_update_descriptor_pool = DescriptorPool(_update_program->setLayouts()[0], n);
			_render_descriptor_pool = DescriptorPool(_render_program->setLayouts()[0], n);
		}

		void createDescriptorSets()
		{
			{
				std::vector<VkDescriptorSetLayout> layouts(_main_window->framesInFlight(), *_update_pipeline.program()->setLayouts().front());
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
					int prev_id = i - 1; if (prev_id < 0)	prev_id = _state_buffers.size() - 1;
					int next_id = i;

					VkDescriptorBufferInfo prev{
						.buffer = _state_buffers[prev_id],
						.offset = 0,
						.range = VK_WHOLE_SIZE,
					};

					VkDescriptorBufferInfo next{
						.buffer = _state_buffers[next_id],
						.offset = 0,
						.range = VK_WHOLE_SIZE,
					};

					VkDescriptorBufferInfo rules{
						.buffer = _rule_buffer,
						.offset = 0,
						.range = VK_WHOLE_SIZE,
					};

					std::array<VkWriteDescriptorSet, 3> descriptor_writes{
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _update_descriptor_sets[i],
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							.pImageInfo = nullptr,
							.pBufferInfo = &prev,
							.pTexelBufferView = nullptr,
						},
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _update_descriptor_sets[i],
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							.pImageInfo = nullptr,
							.pBufferInfo = &next,
							.pTexelBufferView = nullptr,
						},
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _update_descriptor_sets[i],
							.dstBinding = 2,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							.pImageInfo = nullptr,
							.pBufferInfo = &rules,
							.pTexelBufferView = nullptr,
						},
					};

					vkUpdateDescriptorSets(_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
				}
			}
			{
				uint32_t n = _main_window->framesInFlight();
				std::vector<VkDescriptorSetLayout> layouts(n, *_render_pipeline.program()->setLayouts().front());
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
					VkDescriptorBufferInfo state{
						.buffer = _state_buffers[i],
						.offset = 0,
						.range = VK_WHOLE_SIZE,
					};

					VkDescriptorBufferInfo rules{
						.buffer = _rule_buffer,
						.offset = 0,
						.range = VK_WHOLE_SIZE,
					};

					std::array<VkWriteDescriptorSet, 2> descriptor_writes{
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _render_descriptor_sets[i],
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							.pImageInfo = nullptr,
							.pBufferInfo = &state,
						},
						VkWriteDescriptorSet{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.dstSet = _render_descriptor_sets[i],
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							.pImageInfo = nullptr,
							.pBufferInfo = &rules,
							.pTexelBufferView = nullptr,
						},
					};

					vkUpdateDescriptorSets(_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

				}
			}
		}

		void createComputePipeline()
		{
			std::filesystem::path shader_path = std::string(ENGINE_SRC_PATH) + "/src/Particules/update.comp";

			Shader update_shader(this, shader_path, VK_SHADER_STAGE_COMPUTE_BIT, { std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_OF_PARTICULES) });
			_update_program = std::make_shared<ComputeProgram>(std::move(update_shader));
			_update_pipeline = Pipeline(this, _update_program);
		}

		void createGraphicsPipeline()
		{
			std::filesystem::path vert_shader_path = std::string(ENGINE_SRC_PATH) + "/src/Particules/render.vert";
			std::filesystem::path geom_shader_path = std::string(ENGINE_SRC_PATH) + "/src/Particules/render.geom";
			std::filesystem::path frag_shader_path = std::string(ENGINE_SRC_PATH) + "/src/Particules/render.frag";

			std::shared_ptr<Shader> vert = std::make_shared<Shader>(Shader(this, vert_shader_path, VK_SHADER_STAGE_VERTEX_BIT, { std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_OF_PARTICULES) }));
			std::shared_ptr<Shader> geom = std::make_shared<Shader>(Shader(this, geom_shader_path, VK_SHADER_STAGE_GEOMETRY_BIT, { std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_OF_PARTICULES) }));
			std::shared_ptr<Shader> frag = std::make_shared<Shader>(Shader(this, frag_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT, { std::string("N_TYPES_OF_PARTICULES ") + std::to_string(N_TYPES_OF_PARTICULES) }));

			_render_program = std::make_shared<GraphicsProgram>(
				GraphicsProgram::CreateInfo{
					._vertex = vert,
					._geometry = geom,
					._fragment = frag,
				}
			);

			Pipeline::GraphicsCreateInfo ci = {
				._vertex_input = Pipeline::VertexInputWithoutVertices(),
				._input_assembly = Pipeline::InputAssemblyPointDefault(),
				._rasterization = Pipeline::RasterizationDefault(),
				._multisampling = Pipeline::MultisampleOneSample(),
				._render_pass = _render_pass,
				._program = _render_program,
			};

			ci.setViewport({ Pipeline::Viewport(_main_window->extent()) }, { Pipeline::Scissor(_main_window->extent()) });
			ci.setColorBlending({ Pipeline::BlenAttachementNoBlending() });
			ci.assemble();
			_render_pipeline = Pipeline(this, ci);
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
			//ImageView& grid = _grids[grid_id];
			vkResetCommandBuffer(cmd, 0);

			VkCommandBufferBeginInfo begin{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};
			vkBeginCommandBuffer(cmd, &begin);
			{

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

					vkCmdPushConstants(cmd, _render_program->pipelineLayout(), VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(matrix), glm::value_ptr(matrix));

					vkCmdDraw(cmd, _number_of_particules, 1, 0, 0);
				}

				vkCmdEndRenderPass(cmd);
			}
			vkEndCommandBuffer(cmd);
		}

		void recordCommandBufferUpdateAndRender(VkCommandBuffer cmd, size_t grid_id, VkFramebuffer framebuffer, glm::mat4 matrix, float dt)
		{
			vkResetCommandBuffer(cmd, 0);

			VkCommandBufferBeginInfo begin{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};

			vkBeginCommandBuffer(cmd, &begin);
			{

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _update_pipeline);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _update_program->pipelineLayout(), 0, 1, &_update_descriptor_sets[grid_id], 0, nullptr);
				struct pc
				{
					uint32_t n;
					float dt;
				};
				pc _pc;
				_pc.n = _number_of_particules;
				_pc.dt = dt;
				vkCmdPushConstants(cmd, _update_program->pipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &_pc);

				const VkExtent3D dispatch_extent = {.width = (uint32_t)_number_of_particules, .height = 1, .depth = 1};
				const VkExtent3D group_layout = _update_program->localSize();
				vkCmdDispatch(cmd, (dispatch_extent.width + group_layout.width - 1) / group_layout.width, (dispatch_extent.height + group_layout.height - 1) / group_layout.height, (dispatch_extent.depth + group_layout.depth - 1) / group_layout.depth);

				VkClearValue clear_color{ .color = {0, 0, 0, 1} };

				VkRenderPassBeginInfo render_begin{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = _render_pass,
					.framebuffer = framebuffer,
					.renderArea = VkRect2D{.offset = {0, 0}, .extent = _main_window->extent()},
					.clearValueCount = 1,
					.pClearValues = &clear_color,
				};

				VkBufferMemoryBarrier barrier{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.buffer = _state_buffers[grid_id],
					.size = VK_WHOLE_SIZE,
				};

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

				vkCmdBeginRenderPass(cmd, &render_begin, VK_SUBPASS_CONTENTS_INLINE);
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_pipeline);

					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_program->pipelineLayout(), 0, 1, _render_descriptor_sets.data() + grid_id, 0, nullptr);

					vkCmdPushConstants(cmd, _render_program->pipelineLayout(), VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(matrix), glm::value_ptr(matrix));

					vkCmdDraw(cmd, _number_of_particules, 1, 0, 0);
				}

				vkCmdEndRenderPass(cmd);
			}
			VK_CHECK(vkEndCommandBuffer(cmd), "Failed to record a command buffer.");

		}

		void createSemaphores()
		{
			_render_finished_semaphores.resize(_state_buffers.size());

			for (size_t i = 0; i < _render_finished_semaphores.size(); ++i)
			{
				_render_finished_semaphores[i] = Semaphore(this);
			}
		}


	public:

		ParticuleSim(bool validation = false) :
			VkApplication("Particules", validation)
		{
			VkWindow::CreateInfo window_ci{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.in_flight_size = 2,
				.target_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR,
				.name = "Particules",
				.w = 1400,
				.h = 1400,
				.resizeable = GLFW_FALSE,
			};
			_main_window = std::make_shared<VkWindow>(window_ci);

			createRenderPass();
			createFrameBuffers();

			createComputePipeline();
			createGraphicsPipeline();

			createStateBuffers();

			createDescriptorPool();
			createDescriptorSets();

			createCommandBuffers();

			createSemaphores();
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

			{
				VkWindow::AquireResult aquired = _main_window->aquireNextImage();
				VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				recordCommandBufferRenderOnly(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_world_to_cam);
				VkSemaphore wait_semaphore = *aquired.semaphore;
				VkSemaphore render_finished_semaphore = _render_finished_semaphores[aquired.in_flight_index];
				VkSubmitInfo submission{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &wait_semaphore,
					.pWaitDstStageMask = &wait_stage,
					.commandBufferCount = 1,
					.pCommandBuffers = _commands.data() + aquired.in_flight_index,
					.signalSemaphoreCount = 1,
					.pSignalSemaphores = &render_finished_semaphore,
				};
				VkFence submission_fence = *aquired.fence;
				vkQueueSubmit(_queues.graphics, 1, &submission, submission_fence);
				_main_window->present(1, &render_finished_semaphore);
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
					//glm::vec3 screen_mouse_pos_tmp = glm::inverse(screen_coords_matrix * camera.matrix()) * glm::vec3(mouse_handler.currentPosition<float>() - glm::vec2(0.5, 0.5), 1.0);
					//glm::vec2 screen_mouse_pos = glm::vec2(screen_mouse_pos_tmp.x, screen_mouse_pos_tmp.y) / screen_mouse_pos_tmp.z;
					glm::vec2 screen_mouse_pos = (mouse_handler.currentPosition<float>() - glm::vec2(_main_window->extent().width, _main_window->extent().height) * 0.5f) * move_scale;
					camera.zoom(screen_mouse_pos, mouse_handler.getScroll());
					should_render = true;
				}

				mat_world_to_cam = glm::inverse(screen_coords_matrix * camera.matrix());

				if (!paused || should_render)
				{
					VkWindow::AquireResult aquired = _main_window->aquireNextImage();
					VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					if (paused)
					{
						recordCommandBufferRenderOnly(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_world_to_cam);
					}
					else
					{
						recordCommandBufferUpdateAndRender(_commands[aquired.in_flight_index], current_grid_id, _framebuffers[aquired.swap_index], mat_world_to_cam, dt);
					}
					VkSemaphore render_finished_semaphore = _render_finished_semaphores[aquired.in_flight_index];
					VkSemaphore wait_semaphore = *aquired.semaphore;
					VkSubmitInfo submission{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.waitSemaphoreCount = 1,
						.pWaitSemaphores = &wait_semaphore,
						.pWaitDstStageMask = &wait_stage,
						.commandBufferCount = 1,
						.pCommandBuffers = _commands.data() + aquired.in_flight_index,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = &render_finished_semaphore,
					};
					VkFence submission_fence = *aquired.fence;
					vkQueueSubmit(_queues.graphics, 1, &submission, submission_fence);
					_main_window->present(1, &render_finished_semaphore);

					if (!paused)
					{
						current_grid_id = (current_grid_id + 1) % _state_buffers.size();
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