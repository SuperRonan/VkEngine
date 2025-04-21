#define SDL_MAIN_HANDLED

#include <vkl/App/ImGuiApp.hpp>

#include <vkl/VkObjects/VkWindow.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/Buffer.hpp>

#include <vkl/Commands/ComputeCommand.hpp>
#include <vkl/Commands/GraphicsCommand.hpp>
#include <vkl/Commands/TransferCommand.hpp>
#include <vkl/Commands/ImguiCommand.hpp>
#include <vkl/Commands/PrebuiltTransferCommands.hpp>

#include <vkl/Execution/LinearExecutor.hpp>
#include <vkl/Execution/Module.hpp>
#include <vkl/Execution/ResourcesManager.hpp>
#include <vkl/Execution/PerformanceReport.hpp>

#include <vkl/Utils/TickTock.hpp>
#include <vkl/Utils/StatRecorder.hpp>

#include <vkl/IO/ImGuiUtils.hpp>
#include <vkl/IO/InputListener.hpp>

#include <vkl/Rendering/DebugRenderer.hpp>
#include <vkl/Rendering/Camera.hpp>
#include <vkl/Rendering/Model.hpp>
#include <vkl/Rendering/Scene.hpp>
#include <vkl/Rendering/SceneLoader.hpp>
#include <vkl/Rendering/SceneUserInterface.hpp>
#include <vkl/Rendering/ColorCorrection.hpp>
#include <vkl/Rendering/ImagePicker.hpp>
#include <vkl/Rendering/ImageSaver.hpp>

#include <vkl/Maths/Transforms.hpp>
#include <vkl/Maths/AffineXForm.hpp>

#include <argparse/argparse.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include "Renderer.hpp"
#include "PicInPic.hpp"

namespace vkl
{

	class RendererApp : public AppWithImGui
	{
	public:

		virtual std::string getProjectName() const override
		{
			return PROJECT_NAME;
		}

		static void FillArgs(argparse::ArgumentParser& args_parser)
		{
			AppWithImGui::FillArgs(args_parser);
		}

	protected:

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			AppWithImGui::requestFeatures(features);
			features.features2.features.fragmentStoresAndAtomics = VK_TRUE;
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
			features.features2.features.fillModeNonSolid = VK_TRUE;
			features.features_11.multiview = VK_TRUE;
			features.features_12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			features.features_12.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
			features.features_12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
			features.features_12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;

			features.features2.features.shaderInt16 = VK_TRUE;
			features.features_11.storageBuffer16BitAccess = VK_TRUE;

			features.features2.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
			features.features_11.uniformAndStorageBuffer16BitAccess = VK_TRUE;

			features.shader_atomic_float_ext.shaderSharedFloat32AtomicAdd = VK_TRUE;

			//features.compute_shader_derivative_khr.computeDerivativeGroupQuads = VK_TRUE;
		}

		virtual std::set<std::string_view> getDeviceExtensions() override
		{
			std::set<std::string_view> res = AppWithImGui::getDeviceExtensions();
			res.insert(VK_NV_FILL_RECTANGLE_EXTENSION_NAME);
			res.insert(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
			//res.insert(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
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
			scene->setAmbient(Vector3f::Constant(0.02));
			std::shared_ptr<Scene::Node> root = scene->getRootNode();
			if(true)
			{
				std::shared_ptr<RigidMesh> mesh = RigidMesh::MakeSphere(RigidMesh::SphereMakeInfo{
					.app = this,
					.radius = 0.4,
				});

				const size_t n_m = 11;
				const size_t n_r = 11;

				const vec3 base(0, 0, 0);
				const vec3 dir_m(1, 0, 0);
				const vec3 dir_r(0, 0, 1);

				Matrix3x4f node_matrix = TranslationMatrix(vec3(-1, 1, -2)) * (DiagonalMatrix<3, 4>(0.2f));

				std::shared_ptr<Scene::Node> test_material_node = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = "TestMaterials",
					.matrix = node_matrix,
				});

				for (size_t i_m = 0; i_m < n_m; ++i_m)
				{
					const float metallic = float(i_m) / float(n_m - 1);
					for (size_t i_r = 0; i_r < n_r; ++i_r)
					{
						const float roughness = std::pow(float(i_r) / float(n_r - 1), 2);

						const vec3 position = base + float(i_m) * dir_m + float(i_r) * dir_r;

						std::shared_ptr<PBMaterial> material = std::make_shared<PBMaterial>(PBMaterial::CI{
							.app = this,
							.name = std::format("TestMaterial_{0:d}_{1:d}", i_m, i_r),
							.albedo = Vector3f::Constant(0.7).eval(),
							.metallic = metallic,
							.roughness = roughness,
							.cavity = 0,
						});

						std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
							.app = this,
							.name = "TestModel",
							.mesh = mesh,
							.material = material,
						});
						
						std::shared_ptr<Scene::Node> model_node = std::make_shared<Scene::Node>(Scene::Node::CI{
							.name = "ModelNode",
							.matrix = Matrix3x4f(TranslationMatrix(position)),
							.model = model,
						});
						test_material_node->addChild(model_node);
					}
				}
				root->addChild(test_material_node);
			}

			{
				//std::shared_ptr<NodeFromFile> viking_node = std::make_shared<NodeFromFile>(NodeFromFile::CI{
				//	.app = this,
				//	.name = "Vicking",
				//	.matrix = glm::mat4x3(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), Vector3f(1, 0, 0))),
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
				//			.matrix = glm::mat4x3(TranslationMatrix<4, float>(Vector3f(i * 2, 0, j * 2))),
				//		});
				//		node->addChild(viking_node);
				//		root->addChild(node);
				//	}
				//}

				if (true)
				{
					std::shared_ptr<NodeFromFile> sponza_node = std::make_shared<NodeFromFile>(NodeFromFile::CI{
						.app = this,
						.name = "Sponza",
						.matrix = DiagonalMatrix<3, 4>(0.01f),
						.path = "gen:/models/Sponza_2/sponza.obj",
						.synch = false,
					});

					root->addChild(sponza_node);
				}
				
			}

			{
				if (false)
				{
					std::shared_ptr<Scene::Node> light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
						.name = "Light",
						.matrix = Matrix3x4f(TranslationMatrix(Vector3f(1, 6, -1))),
					});
					light_node->light() = std::make_shared<DirectionalLight>(DirectionalLight::CI{
						.app = this,
						.name = "Light",
						.direction = Vector3f(1, 1, -1),
						.emission = Vector3f(1, 0.8, 0.6),
					});
					root->addChild(light_node);
				}
				else
				{
					std::shared_ptr<PointLight> pl = std::make_shared<PointLight>(PointLight::CI{
						.app = this,
						.name = "PointLight",
						.position = Vector3f(0, 0, 0),
						.emission = Vector3f(1, 0.8, 0.6) * 15.0f,
						.enable_shadow_map = true,
						
					});
					int n_lights = 3;
					for (int i = 0; i < n_lights; ++i)
					{
						std::shared_ptr<Scene::Node> light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
							.name = "PointLight" + std::to_string(i),
							.matrix = TranslationMatrix(Vector3f((i - (n_lights / 2)) * 6, 6, 0)),
						});
						light_node->light() = pl;
						root->addChild(light_node);
					}
				}

				//light_node->light() = std::make_shared<Light>(Light::MakePoint(Vector3f(0), Vector3f(1, 1, 1)));


				for (int i = 0; i < 3; ++i)
				{
					Vector3f color = Vector3f::Zero();
					color[i] = 10 * 5 * 2;
					std::shared_ptr<SpotLight> spot_light = std::make_shared<SpotLight>(SpotLight::CI{
						.app = this,
						.name = "SpotLight" + std::to_string(i),
						.direction = Vector3f(0, 0, -1),
						.up = Vector3f(0, 1, 0),
						.emission = color,
						.attenuation = 1,
					});
					vec3 position = Vector3f(2, 1, -4 + 4);
					if (i == 1)
					{
						position += Vector3f(0.5, 0, 0);
					}
					else if (i == 2)
					{
						position += Vector3f(0.25, sqrt(3.0) / 2.0 * 0.5, 0);
					}
					std::shared_ptr<Scene::Node> spot_light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
						.name = "SpotLight" + std::to_string(i),
						.matrix = TranslationMatrix(position),
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
			_desired_window_options.present_mode = VK_PRESENT_MODE_FIFO_KHR;
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
				.common_definitions = resources_manager.commonDefinitions(),
				.use_ImGui = true,
				.use_debug_renderer = true,
			});

			Camera camera(Camera::CreateInfo{
				.resolution = _main_window->extent2D(),
				.znear = 0.01,
				.zfar = 100,
			});
			camera.update(Camera::CameraDelta{
				.angle = Vector2f(std::numbers::pi, 0),
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
				.camera = &camera,
			});
			if (exec.getDebugRenderer())
			{
				exec.getDebugRenderer()->setTargets(final_image, renderer.depth());
			}

			ColorCorrection color_correction = ColorCorrection::CI{
				.app = this,
				.name = "ColorCorrection",
				.dst = final_image,
				.sets_layouts = sets_layouts,
				.target_window = _main_window,
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
				.dst_folder = "gen:/saved_images/",
				.dst_filename = "renderer_",
			};

			std::shared_ptr<ComputeCommand> slang_test;
			if (false)
			{
				slang_test = std::make_shared<ComputeCommand>(ComputeCommand::CI{
					.app = this,
					.name = "SlangTest",
					.shader_path = "ProjectShaders:/test.slang",
					.extent = final_image->image()->extent(),
					.dispatch_threads = true,
					.sets_layouts = sets_layouts,
					.bindings = {
						Binding{
							.image = final_image,
							.binding = 0,
						},
					},
					.definitions = [&](DefinitionsList& res) {
						res.clear();
						DetailedVkFormat df = DetailedVkFormat::Find(final_image->format().value());
						res.pushBackFormatted("TARGET_FORMAT {}", df.getGLSLName());
					},
				});
			}

			exec.init();

			
			std::shared_ptr<PerformanceReport> perf_reporter = std::make_shared<PerformanceReport>(PerformanceReport::CI{
				.app = this,
				.name = "Performances",
				.period = 1s,
				.memory = 256,
			});
			FramePerfCounters & frame_counters = perf_reporter->framePerfCounter();

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

			

			FirstPersonCameraController camera_controller(FirstPersonCameraController::CreateInfo{
				.camera = &camera, 
				.keyboard = &keyboard,
				.mouse = &mouse,
				.gamepad = &gamepad,
			});

			std::TickTock_hrc frame_tick_tock;
			frame_tick_tock.tick();

			std::TickTock_hrc tt;
			bool log = false;

			const int flip_imgui_key = SDL_SCANCODE_F1;
			while (!_main_window->shouldClose())
			{
				{
					double new_t = std::chrono::duration<double>(clock.now() - clock_zero).count();
					dt = new_t - t;
					t = new_t;
				}
				const ImGuiIO * imgui_io = ImGuiIsInit() ?  &ImGui::GetIO() : nullptr;
				{
					tt.tick();
					SDL_Event event;
					while (SDL_PollEvent(&event))
					{
						bool forward_mouse_event = true;
						bool forward_keyboard_event = true;
						if (ImGuiIsEnabled())
						{
							ImGui_ImplSDL3_ProcessEvent(&event);
							forward_mouse_event = !imgui_io->WantCaptureMouse;
							forward_keyboard_event = !imgui_io->WantCaptureKeyboard;
						}
						
						if(forward_mouse_event)
						{
							mouse.processEventCheckRelevent(event);
						}
						gamepad.processEventCheckRelevent(event);
						_main_window->processEventCheckRelevent(event);
					}
					tt.tock();
					if (log)
					{
						std::chrono::microseconds waited = std::chrono::duration_cast<std::chrono::microseconds>(tt.duration());
						logger()(std::format("Total PollEvent time: {}us", waited.count()), Logger::Options::TagInfo);
					}
				}

				if (imgui_io && !imgui_io->WantCaptureKeyboard)
				{
					keyboard.update();
				}
				mouse.update();
				gamepad.update();
				camera_controller.updateCamera(dt);

				bool flip_imgui_enable = keyboard.getKey(flip_imgui_key).justReleased();
				if (flip_imgui_enable)
				{
					AppWithImGui::_enable_imgui = !AppWithImGui::_enable_imgui;
				}
				
				
				if (ImGuiIsEnabled())
				{
					GuiContext * gui_ctx = beginImGuiFrame();
					
					//ImGui::ShowDemoWindow();
					if(ImGui::Begin("Rendering"))
					{
						camera.declareGui(*gui_ctx);

						renderer.declareGui(*gui_ctx);

						color_correction.declareGui(*gui_ctx);

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
						perf_reporter->declareGUI(*gui_ctx);
					}
					ImGui::End();

					if (ImGui::Begin("Display"))
					{
						_main_window->declareGui(*gui_ctx);
					}
					ImGui::End();
					
					endImGuiFrame(gui_ctx);
				}
				else if (flip_imgui_enable)
				{
					GuiContext* gui_ctx = beginImGuiFrame();
					endImGuiFrame(gui_ctx);
				}

				

				if(mouse.getButton(SDL_BUTTON_RIGHT).justReleased())
				{
					pip.setPosition(mouse.getReleasedPos(SDL_BUTTON_RIGHT) / Vector2f(_main_window->extent2D().value().width, _main_window->extent2D().value().height));
				}

				frame_counters.reset();

				std::shared_ptr<UpdateContext> update_context = resources_manager.beginUpdateCycle();
				{
					getSamplerLibrary().updateResources(*update_context);
					{
						std::TickTock_hrc prepare_scene_tt;
						prepare_scene_tt.tick();
						scene->updateInternal();
						frame_counters.prepare_scene_time = prepare_scene_tt.tockd().count();
					}

					exec.updateResources(*update_context);
					renderer.preUpdate(*update_context);
					{
						std::TickTock_hrc update_scene_tt;
						update_scene_tt.tick();
						scene->updateResources(*update_context);
						frame_counters.update_scene_time = update_scene_tt.tockd().count();
					}
					renderer.updateResources(*update_context);
					color_correction.updateResources(*update_context);
					pip.updateResources(*update_context);
					image_picker.updateResources(*update_context);
					image_saver.updateResources(*update_context);
					sui->updateResources(*update_context);
					script_resources.update(*update_context);

					if (slang_test)
					{
						update_context->resourcesToUpdateLater() += slang_test;
					}

					resources_manager.finishUpdateCycle(update_context);
				}
				frame_counters.update_time = update_context->tickTock().tockd().count();
				
				std::TickTock_hrc render_tick_tock;
				render_tick_tock.tick();
				{	
					exec.beginFrame(perf_reporter->generateFrameReport());
					exec.performSynchTransfers(*update_context, true);
					
					ExecutionThread* ptr_exec_thread = exec.beginCommandBuffer();
					ExecutionThread& exec_thread = *ptr_exec_thread;
					exec_thread.setFramePerfCounters(&frame_counters);

					
					exec.performAsynchMipsCompute(*update_context->mipsQueue());

					exec_thread.bindSet(BindSetInfo{
						.index = 1, 
						.set = scene->set(),
					});
					scene->prepareForRendering(exec_thread);
					renderer.execute(exec_thread, t, dt, frame_index);

					image_picker.execute(exec_thread);

					sui->execute(exec_thread, camera);

					color_correction.execute(exec_thread);
					
					pip.execute(exec_thread);

					if (slang_test)
					{
						exec_thread(slang_test);
					}

					exec_thread.bindSet(BindSetInfo{
						.index = 1,
						.set = nullptr,
					});

					exec.renderDebugIFP();
					image_saver.execute(exec_thread);
					exec.endCommandBuffer(ptr_exec_thread);

					ptr_exec_thread = exec.beginCommandBuffer(false);
					tt.tick();
					exec.aquireSwapchainImage();
					tt.tock();
					if (log)
					{
						std::chrono::microseconds waited = std::chrono::duration_cast<std::chrono::microseconds>(tt.duration());
						logger()(std::format("Total Aquire time: {}us", waited.count()), Logger::Options::TagInfo);
					}
					exec.preparePresentation(final_image, ImGuiIsEnabled());
					exec.endCommandBuffer(ptr_exec_thread);
					
					tt.tick();
					exec.submit();
					tt.tock();
					if (log)
					{
						std::chrono::microseconds waited = std::chrono::duration_cast<std::chrono::microseconds>(tt.duration());
						logger()(std::format("Total Submit time: {}us", waited.count()), Logger::Options::TagInfo);
					}

					tt.tick();
					exec.present();
					tt.tock();
					if (log)
					{
						std::chrono::microseconds waited = std::chrono::duration_cast<std::chrono::microseconds>(tt.duration());
						logger()(std::format("Total Present time: {}us", waited.count()), Logger::Options::TagInfo);
					}

					exec.endFrame();


					++frame_index;
				}
				frame_counters.render_time = render_tick_tock.tockd().count();
				frame_counters.frame_time = frame_tick_tock.tockd().count();
				frame_tick_tock.tick();
				{
					perf_reporter->setFramePerfReport(exec.getPendingFrameReport());
					perf_reporter->advance();
				}
			}
			exec.waitForAllCompletion();

			VK_CHECK(deviceWaitIdle(), "Failed to wait for completion.");

			VKL_BREAKPOINT_HANDLE;
		}

	};
}


int main(int argc, char** argv)
{
#ifdef _WINDOWS
	bool can_utf8 = SetConsoleOutputCP(CP_UTF8);
#endif
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