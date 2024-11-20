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

#include <argparse/argparse.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include "BSDFViewer.hpp"

#include "PicInPic.hpp"

namespace vkl
{
	class BSDFApp : public AppWithImGui
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
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
			features.features2.features.fillModeNonSolid = VK_TRUE;
			features.features_11.multiview = VK_TRUE;

			features.shader_atomic_float_ext.shaderBufferFloat32AtomicAdd = VK_TRUE;
			features.shader_atomic_float_ext.shaderBufferFloat64AtomicAdd = VK_TRUE;
		}

		virtual std::set<std::string_view> getDeviceExtensions() override
		{
			std::set<std::string_view> res = AppWithImGui::getDeviceExtensions();
			res.insert(VK_NV_FILL_RECTANGLE_EXTENSION_NAME);
			res.insert(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
			return res;
		}

	public:

		struct CreateInfo
		{

		};
		using CI = CreateInfo;
		
		BSDFApp(std::string const& name, argparse::ArgumentParser & args):
			AppWithImGui(AppWithImGui::CI{
				.name = name,
				.args = args,
			})
		{

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
				.common_definitions = resources_manager.commonDefinitions(),
				.use_ImGui = true,
				.use_debug_renderer = true,
			});

			const std::filesystem::path shaders = "ProjectShaders:/";

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

			std::shared_ptr<DescriptorSetLayout> common_layout = exec.getCommonSetLayout();
			MultiDescriptorSetsLayouts sets_layouts;
			sets_layouts += {0, common_layout};

			if (exec.getDebugRenderer())
			{
				exec.getDebugRenderer()->setTargets(final_image);
			}

			Camera camera(Camera::CreateInfo{
				.resolution = _main_window->extent2D(),
				.znear = 0.01,
				.zfar = 100,
			});

			BSDFViewer bsdf_viewer = BSDFViewer::CI{
				.app = this,
				.name = "BSDF_Viewer",
				.target = final_image,
				.camera = &camera,
				.sets_layouts = sets_layouts
			};

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

			ImageSaver image_saver = ImageSaver::CI{
				.app = this,
				.name = "ImageSaver",
				.src = final_image,
				.dst_folder = "gen:/saved_images/",
				.dst_filename = "bsdf_",
			};

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
						ImGui_ImplSDL3_ProcessEvent(&event);
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
					//ImGui::ShowDemoWindow();

					if(ImGui::Begin("Rendering"))
					{
						if (ImGui::CollapsingHeader("BSDF Viewer"))
						{
							bsdf_viewer.declareGUI(*gui_ctx);
						}

						camera.declareGui(*gui_ctx);

						color_correction.declareGui(*gui_ctx);

						pip.declareGui(*gui_ctx);

						image_saver.declareGUI(*gui_ctx);

						if (exec.getDebugRenderer())
						{
							exec.getDebugRenderer()->declareGui(*gui_ctx);
						}

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

				if(mouse.getButton(SDL_BUTTON_RIGHT).justReleased())
				{
					pip.setPosition(mouse.getReleasedPos(SDL_BUTTON_RIGHT) / glm::vec2(_main_window->extent2D().value().width, _main_window->extent2D().value().height));
				}

				frame_counters.reset();

				std::shared_ptr<UpdateContext> update_context = resources_manager.beginUpdateCycle();
				{
					getSamplerLibrary().updateResources(*update_context);

					exec.updateResources(*update_context);

					bsdf_viewer.updateResources(*update_context);

					color_correction.updateResources(*update_context);
					pip.updateResources(*update_context);
					image_saver.updateResources(*update_context);
					script_resources.update(*update_context);

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

					bsdf_viewer.execute(exec_thread);

					color_correction.execute(exec_thread);
					 
					pip.execute(exec_thread);

					exec.renderDebugIFP();
					image_saver.execute(exec_thread);
					exec.endCommandBuffer(ptr_exec_thread);

					ptr_exec_thread = exec.beginCommandBuffer(false);
					exec.aquireSwapchainImage();
					exec.preparePresentation(final_image);
					
					
					exec.endCommandBuffer(ptr_exec_thread);
					exec.submit();
					exec.present();
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

			VK_CHECK(vkDeviceWaitIdle(_device), "Failed to wait for completion.");

			VKL_BREAKPOINT_HANDLE;
		}

	};
}


int main(int argc, char** argv)
{
	try
	{
		argparse::ArgumentParser args;
		vkl::BSDFApp::FillArgs(args);
		
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

		vkl::BSDFApp app(PROJECT_NAME, args);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}