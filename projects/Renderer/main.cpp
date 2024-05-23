#define SDL_MAIN_HANDLED

#include <Core/App/ImGuiApp.hpp>

#include <Core/VkObjects/VkWindow.hpp>
#include <Core/VkObjects/ImageView.hpp>
#include <Core/VkObjects/Buffer.hpp>

#include <Core/Commands/ComputeCommand.hpp>
#include <Core/Commands/GraphicsCommand.hpp>
#include <Core/Commands/TransferCommand.hpp>
#include <Core/Commands/ImguiCommand.hpp>

#include <Core/Execution/LinearExecutor.hpp>
#include <Core/Execution/Module.hpp>
#include <Core/Execution/ResourcesManager.hpp>
#include <Core/Execution/FramePerformanceCounters.hpp>

#include <Core/Utils/TickTock.hpp>
#include <Core/Utils/StatRecorder.hpp>

#include <Core/IO/ImGuiUtils.hpp>
#include <Core/IO/InputListener.hpp>

#include <Core/Rendering/DebugRenderer.hpp>
#include <Core/Rendering/Camera.hpp>
#include <Core/Rendering/Model.hpp>
#include <Core/Rendering/Scene.hpp>
#include <Core/Rendering/SceneLoader.hpp>
#include <Core/Rendering/SceneUserInterface.hpp>
#include <Core/Rendering/GammaCorrection.hpp>
#include <Core/Rendering/ImagePicker.hpp>
#include <Core/Rendering/ImageSaver.hpp>

#include <Core/Maths/Transforms.hpp>

#include <argparse/argparse.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include "Renderer.hpp"
#include "PicInPic.hpp"

namespace vkl
{
	const char* GetProjectName()
	{
		return PROJECT_NAME;
	}

	class RendererApp : public AppWithImGui
	{
	public:

		static void FillArgs(argparse::ArgumentParser& args_parser)
		{
			AppWithImGui::FillArgs(args_parser);
		}

	protected:

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			AppWithImGui::requestFeatures(features);
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
			features.features2.features.fillModeNonSolid = VK_TRUE;
			features.features_11.multiview = VK_TRUE;
		}

		virtual std::set<std::string_view> getDeviceExtensions() override
		{
			std::set<std::string_view> res = AppWithImGui::getDeviceExtensions();
			res.insert(VK_NV_FILL_RECTANGLE_EXTENSION_NAME);
			return res;
		}

	public:

		struct CreateInfo
		{

		};
		using CI = CreateInfo;
		
		RendererApp(std::string const& name, argparse::ArgumentParser & args):
			AppWithImGui(AppWithImGui::CI{
				.name = name,
				.args = args,
			})
		{

		}

		void createScene(std::shared_ptr<Scene> & scene)
		{
			std::shared_ptr<Scene::Node> root = scene->getRootNode();

			{
				std::shared_ptr<RigidMesh> mesh = RigidMesh::MakeSphere(RigidMesh::SphereMakeInfo{
					.app = this,
					.radius = 0.5,
				});

				std::shared_ptr<PBMaterial> material = std::make_shared<PBMaterial>(PBMaterial::CI{
					.app = this,
					.name = "DefaultMaterial",
				});

				std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
					.app = this,
					.name = "Model",
					.mesh = mesh,
					.material = material,
				});

				std::shared_ptr<Scene::Node> model_node = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = "ModelNode",
					.matrix = glm::mat4x3(translateMatrix<4, float>(glm::vec3(1, 1, 1)) * scaleMatrix<4, float>(0.5)),
					.model = model,
				});
				root->addChild(model_node);

				root->addChild(std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = "ModelNode2",
					.matrix = glm::mat4x3(translateMatrix<4, float>(glm::vec3(2, 1, 1)) * scaleMatrix<4, float>(0.5)),
					.model = model,
				}));
			}

			{
				//std::shared_ptr<NodeFromFile> viking_node = std::make_shared<NodeFromFile>(NodeFromFile::CI{
				//	.app = this,
				//	.name = "Vicking",
				//	.matrix = glm::mat4x3(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0))),
				//	.path = ENGINE_SRC_PATH "/../gen/models/viking_room/viking_room.obj",
				//	.synch = false,
				//});

				const int N = 1;

				//for(int i=-N; i<= N; ++i)
				//{
				//	for (int j = -N; j <= N; ++j)
				//	{
				//		std::shared_ptr<Scene::Node> node = std::make_shared<Scene::Node>(Scene::Node::CI{
				//			.name = "translate(" + std::to_string(i) + ", " + std::to_string(j) + ")",
				//			.matrix = glm::mat4x3(translateMatrix<4, float>(Vector3f(i * 2, 0, j * 2))),
				//		});
				//		node->addChild(viking_node);
				//		root->addChild(node);
				//	}
				//}

				std::shared_ptr<NodeFromFile> sponza_node = std::make_shared<NodeFromFile>(NodeFromFile::CI{
					.app = this,
					.name = "Sponza",
					.matrix = scaleMatrix<4, float>(0.01),
					.path = ENGINE_SRC_PATH "/../gen/models/Sponza_2/sponza.obj",
					.synch = false,
				});

				root->addChild(sponza_node);
				
			}

			{
				if (false)
				{
					std::shared_ptr<Scene::Node> light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
						.name = "Light",
						.matrix = glm::mat4x3(translateMatrix<4, float>(glm::vec3(1, 6, -1))),
					});
					light_node->light() = std::make_shared<DirectionalLight>(DirectionalLight::CI{
						.app = this,
						.name = "Light",
						.direction = glm::vec3(1, 1, -1),
						.emission = glm::vec3(1, 0.8, 0.6),
					});
					root->addChild(light_node);
				}
				else
				{
					std::shared_ptr<PointLight> pl = std::make_shared<PointLight>(PointLight::CI{
						.app = this,
						.name = "PointLight",
						.position = glm::vec3(0, 0, 0),
						.emission = glm::vec3(1, 0.8, 0.6) * 15.0f,
						.enable_shadow_map = true,
						
					});
					int n_lights = 3;
					for (int i = 0; i < n_lights; ++i)
					{
						std::shared_ptr<Scene::Node> light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
							.name = "PointLight" + std::to_string(i),
							.matrix = glm::mat4x3(translateMatrix<4, float>(glm::vec3((i - (n_lights / 2)) * 6, 6, 0))),
						});
						light_node->light() = pl;
						root->addChild(light_node);
					}
				}

				//light_node->light() = std::make_shared<Light>(Light::MakePoint(glm::vec3(0), glm::vec3(1, 1, 1)));


				for (int i = 0; i < 3; ++i)
				{
					glm::vec3 color = glm::vec3(0);
					color[i] = 10;
					std::shared_ptr<SpotLight> spot_light = std::make_shared<SpotLight>(SpotLight::CI{
						.app = this,
						.name = "SpotLight" + std::to_string(i),
						.direction = glm::vec3(0, 0, -1),
						.up = glm::vec3(0, 1, 0),
						.emission = color,
						.attenuation = 1,
					});
					vec3 position = glm::vec3(2, 1, -4 + 4);
					if (i == 1)
					{
						position += glm::vec3(0.5, 0, 0);
					}
					else if (i == 2)
					{
						position += glm::vec3(0.25, sqrt(3.0) / 2.0 * 0.5, 0);
					}
					std::shared_ptr<Scene::Node> spot_light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
						.name = "SpotLight" + std::to_string(i),
						.matrix = glm::mat4x3(translateMatrix<4, float>(position)),
					});
					spot_light_node->light() = spot_light;
					root->addChild(spot_light_node);
				}

			}
		}

		virtual void run() final override
		{
			VkApplication::init();

			_desired_window_options.name = PROJECT_NAME;
			_desired_window_options.queue_families_indices = std::set<uint32_t>({desiredQueuesIndices()[0].family});
			_desired_window_options.resizeable = true;
			//_desired_window_options.mode = VkWindow::Mode::WindowedFullscreen;
			createMainWindow();
			initImGui();

			ResourcesLists script_resources;

			ResourcesManager resources_manager = ResourcesManager::CreateInfo{
				.app = this,
				.name = "ResourcesManager",
				.shader_check_period = 1s,
			};

			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "exec",
				.window = _main_window,
				.mounting_points = resources_manager.mountingPoints(),
				.common_definitions = resources_manager.commonDefinitions(),
				.use_ImGui = true,
				.use_debug_renderer = true,
			});

			VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
			//VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
			std::shared_ptr<ImageView> final_image = std::make_shared<ImageView>(Image::CI{
				.app = this,
				.name = "Final Image",
				.type = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = _main_window->extent3D(),
				.usage = VK_IMAGE_USAGE_TRANSFER_BITS | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
			});
			script_resources += final_image;

			std::shared_ptr<Scene> scene = std::make_shared<Scene>(Scene::CI{
				.app = this,
				.name = "scene",
			});

			createScene(scene);

			std::shared_ptr<DescriptorSetLayout> common_layout = exec.getCommonSetLayout();
			std::shared_ptr<DescriptorSetLayout> scene_layout = scene->setLayout();
			MultiDescriptorSetsLayouts sets_layouts;
			sets_layouts += {0, common_layout};
			sets_layouts += {1, scene_layout};

			SimpleRenderer renderer(SimpleRenderer::CI{
				.app = this,
				.name = "Renderer",
				.sets_layouts = sets_layouts,
				.scene = scene,
				.target = final_image,
			});
			if (exec.getDebugRenderer())
			{
				exec.getDebugRenderer()->setTargets(final_image, renderer.depth());
			}

			GammaCorrection gamma_correction = GammaCorrection::CI{
				.app = this,
				.name = "GammaCorrection",
				.dst = final_image,
				.sets_layouts = sets_layouts,
				.swapchain = _main_window->swapchain(),
			};

			PictureInPicture pip = PictureInPicture::CI{
				.app = this,
				.name = "PiP",
				.target = final_image,
				.sets_layouts = sets_layouts,
			};

			std::shared_ptr<SceneUserInterface> sui = std::make_shared<SceneUserInterface>(SceneUserInterface::CI{
				.app = this,
				.name = "SceneUserInterface",
				.scene = scene,
				.target = final_image,
				.depth = nullptr,
				.sets_layouts = sets_layouts,
			});

			ImagePicker image_picker = ImagePicker::CI{
				.app = this,
				.name = "ImagePicker",
				.sources = {
					renderer.output(),
					renderer.renderTarget(),
					renderer.getAmbientOcclusionTargetIFP(),
					renderer.getAlbedoImage(),
					renderer.getPositionImage(),
					renderer.getNormalImage(),
					renderer.depth(), // Does not work yet
				},
				.dst = final_image,
			};

			ImageSaver image_saver = ImageSaver::CI{
				.app = this,
				.name = "ImageSaver",
				.src = final_image,
				.dst_folder = _mounting_points["gen"] + "/saved_images/",
				.dst_filename = "renderer_",
			};

			exec.init();

			FramePerfCounters frame_counters;
			StatRecords stat_records = StatRecords::CI{
				.name = "Statistics",
				.memory = 256,
				.period = 1s,
			};
			stat_records.createCommonRecords(frame_counters);

			//for (int jid = 0; jid <= GLFW_JOYSTICK_LAST; ++jid)
			//{
			//	if (glfwJoystickPresent(jid))
			//	{
			//		std::cout << "Joystick " << jid << ": " << glfwGetJoystickName(jid) << std::endl;
			//	}
			//}

			KeyboardStateListener keyboard;
			MouseEventListener mouse;
			GamepadListener gamepad;

			std::chrono::steady_clock clock;
			decltype(clock)::time_point clock_zero = clock.now();

			double t = 0, dt = 0.0;
			size_t frame_index = 0;

			Camera camera(Camera::CreateInfo{
				.resolution = _main_window->extent2D(),
				.znear = 0.01,
				.zfar = 100,
			});

			FirstPersonCameraController camera_controller(FirstPersonCameraController::CreateInfo{
				.camera = &camera, 
				.keyboard = &keyboard,
				.mouse = &mouse,
				.gamepad = &gamepad,
			});

			std::TickTock_hrc frame_tick_tock;
			frame_tick_tock.tick();

			while (!_main_window->shouldClose())
			{
				{
					double new_t = std::chrono::duration<double>(clock.now() - clock_zero).count();
					dt = new_t - t;
					t = new_t;
				}
				const auto& imgui_io = ImGui::GetIO();
				{
					SDL_Event event;
					while (SDL_PollEvent(&event))
					{
						ImGui_ImplSDL2_ProcessEvent(&event);
						if(!imgui_io.WantCaptureKeyboard)
						{
						}
						if(!imgui_io.WantCaptureMouse)
						{
							mouse.processEventCheckRelevent(event);
						}
						gamepad.processEventCheckRelevent(event);
						_main_window->processEventCheckRelevent(event);
					}
				}

				if (!imgui_io.WantCaptureKeyboard)
				{
					keyboard.update();
				}
				mouse.update();
				gamepad.update();
				camera_controller.updateCamera(dt);

				{
					GuiContext * gui_ctx = beginImGuiFrame();
					ImGui::ShowDemoWindow();

					if(ImGui::Begin("Rendering"))
					{
						camera.declareGui(*gui_ctx);

						renderer.declareGui(*gui_ctx);

						gamma_correction.declareGui(*gui_ctx);

						pip.declareGui(*gui_ctx);

						image_picker.declareGUI(*gui_ctx);

						image_saver.declareGUI(*gui_ctx);

						if (exec.getDebugRenderer())
						{
							exec.getDebugRenderer()->declareGui(*gui_ctx);
						}

					}
					ImGui::End();

					if(ImGui::Begin("Scene"))
					{
						sui->declareGui(*gui_ctx);

					}
					ImGui::End();

					if(ImGui::Begin("Performances"))
					{
						stat_records.declareGui(*gui_ctx);
					}
					ImGui::End();

					if (ImGui::Begin("Display"))
					{
						_main_window->declareGui(*gui_ctx);
					}
					ImGui::End();
					endImGuiFrame(gui_ctx);
				}

				if(mouse.getButton(SDL_BUTTON_RIGHT).justReleased())
				{
					pip.setPosition(mouse.getReleasedPos(SDL_BUTTON_RIGHT) / glm::vec2(_main_window->extent2D().value().width, _main_window->extent2D().value().height));
				}

				frame_counters.reset();

				std::shared_ptr<UpdateContext> update_context = resources_manager.beginUpdateCycle();
				{
					getSamplerLibrary().updateResources(*update_context);
					{
						std::TickTock_hrc prepare_scene_tt;
						prepare_scene_tt.tick();
						scene->updateInternal();
						frame_counters.prepare_scene_time = prepare_scene_tt.tockv().count();
					}

					exec.updateResources(*update_context);
					renderer.preUpdate(*update_context);
					{
						std::TickTock_hrc update_scene_tt;
						update_scene_tt.tick();
						scene->updateResources(*update_context);
						frame_counters.update_scene_time = update_scene_tt.tockv().count();
					}
					renderer.updateResources(*update_context);
					gamma_correction.updateResources(*update_context);
					pip.updateResources(*update_context);
					image_picker.updateResources(*update_context);
					image_saver.updateResources(*update_context);
					sui->updateResources(*update_context);
					script_resources.update(*update_context);

					resources_manager.finishUpdateCycle(update_context);
				}
				frame_counters.update_time = update_context->tickTock().tockv().count();
				
				std::TickTock_hrc render_tick_tock;
				render_tick_tock.tick();
				{	
					exec.performSynchTransfers(*update_context, true);
					
					ExecutionThread* ptr_exec_thread = exec.beginCommandBuffer();
					ExecutionThread& exec_thread = *ptr_exec_thread;
					exec_thread.setFramePerfCounters(&frame_counters);

					
					exec.performAsynchMipsCompute(*update_context->mipsQueue());

					exec_thread.bindSet(1, scene->set());
					scene->prepareForRendering(exec_thread);
					renderer.execute(exec_thread, camera, t, dt, frame_index);

					image_picker.execute(exec_thread);

					sui->execute(exec_thread, camera);

					gamma_correction.execute(exec_thread);
					 
					pip.execute(exec_thread);

					exec_thread.bindSet(1, nullptr);

					exec.renderDebugIFP();
					image_saver.execute(exec_thread);
					exec.endCommandBuffer(ptr_exec_thread);

					ptr_exec_thread = exec.beginCommandBuffer(false);
					exec.AquireSwapchainImage();
					exec.preparePresentation(final_image);
					
					
					exec.endCommandBuffer(ptr_exec_thread);
					exec.submit();
					exec.present();


					++frame_index;
				}
				frame_counters.render_time = render_tick_tock.tockv().count();
				frame_counters.frame_time = frame_tick_tock.tockv().count();
				frame_tick_tock.tick();
				{
					stat_records.advance();
				}
			}

			exec.waitForAllCompletion();

			VK_CHECK(vkDeviceWaitIdle(_device), "Failed to wait for completion.");

		}

	};
}


int main(int argc, char** argv)
{
	try
	{
		argparse::ArgumentParser args;
		vkl::RendererApp::FillArgs(args);
		
		try 
		{
			args.parse_args(argc, argv);
		}
		catch (std::exception const& e)
		{
			std::cerr << e.what() << std::endl;
			std::cerr << args << std::endl;
			return -1;
		}

		vkl::RendererApp app(PROJECT_NAME, args);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}