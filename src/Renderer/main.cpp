#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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

#include <Core/IO/ImGuiUtils.hpp>
#include <Core/IO/InputListener.hpp>

#include <Core/Rendering/DebugRenderer.hpp>
#include <Core/Rendering/RenderObjects.hpp>
#include <Core/Rendering/Model.hpp>
#include <Core/Rendering/Scene.hpp>
#include <Core/Rendering/Transforms.hpp>

#include <iostream>
#include <chrono>
#include <random>

#include "Renderer.hpp"
#include "PicInPic.hpp"

namespace vkl
{
	class RendererApp : public AppWithWithImGui
	{
	protected:

		virtual void requestFeatures(VulkanFeatures& features) override
		{
			AppWithWithImGui::requestFeatures(features);
			features.features_12.separateDepthStencilLayouts = VK_TRUE;
		}

	public:

		RendererApp(bool enable_validation):
			AppWithWithImGui(AppWithWithImGui::CI{
				.name = PROJECT_NAME,
				.enable_validation = enable_validation
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
			}

			{
				std::vector<std::shared_ptr<Model>> viking_models = Model::loadModelsFromObj(Model::LoadInfo{
					.app = this,
					.path = ENGINE_SRC_PATH "/../gen/models/viking_room.obj",
				});

				std::shared_ptr<Scene::Node> viking_node = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = "Viking",
					.matrix = glm::mat4x3(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0))),
					.model = viking_models.front(),
				});

				const int N = 1;

				for(int i=-N; i<= N; ++i)
				{
					for (int j = -N; j <= N; ++j)
					{
						std::shared_ptr<Scene::Node> node = std::make_shared<Scene::Node>(Scene::Node::CI{
							.name = "translate(" + std::to_string(i) + ", " + std::to_string(j) + ")",
							.matrix = glm::mat4x3(translateMatrix<4, float>(Vector3f(i * 2, 0, j * 2))),
						});
						node->addChild(viking_node);
						root->addChild(node);
					}
				}

				
			}

			{
				std::shared_ptr<Scene::Node> light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = "Light",
					.matrix = glm::mat4x3(translateMatrix<4, float>(glm::vec3(1, 1, -1))),
				});

				//light_node->light() = std::make_shared<Light>(Light::MakePoint(glm::vec3(0), glm::vec3(1, 1, 1)));
				light_node->light() = std::make_shared<DirectionalLight>(DirectionalLight::CI{
					.app = this,
					.name = "Light",
					.direction = glm::vec3(1, 1, -1),
					.emission = glm::vec3(1, 0.8, 0.6),
				});

				root->addChild(light_node);
			}
		}

		virtual void run() final override
		{
			VkApplication::init();

			std::shared_ptr<VkWindow> window = std::make_shared<VkWindow>(VkWindow::CreateInfo{
				.app = this,
				.queue_families_indices = std::set({_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()}),
				.target_present_mode = VK_PRESENT_MODE_FIFO_KHR,
				.name = PROJECT_NAME,
				.w = 1600,
				.h = 900,
				.resizeable = GLFW_TRUE,
			});
			initImGui(window);

			std::filesystem::path shaders = PROJECT_SRC_PATH;

			ResourcesLists script_resources;
			MountingPoints mounting_points;

			LinearExecutor exec(LinearExecutor::CI{
				.app = this,
				.name = "exec",
				.window = window,
				.mounting_points = &mounting_points,
				.use_ImGui = true,
			});

			ResourcesManager resources_manager = ResourcesManager::CreateInfo{
				.app = this,
				.name = "ResourcesManager",
				.shader_check_period = 1s,
				.common_definitions = &exec.getCommonDefinitions(),
				.mounting_points = &mounting_points,
			};

			std::shared_ptr<ImageView> final_image = std::make_shared<ImageView>(ImageView::CI{
				.name = "Final Image View",
				.image_ci = Image::CI{
					.app = this,
					.name = "Final Image",
					.type = VK_IMAGE_TYPE_2D,
					.format = VK_FORMAT_R16G16B16A16_SFLOAT,
					.extent = window->extent3D(),
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					.mem_usage = VMA_MEMORY_USAGE_GPU_ONLY,
				},
			});
			script_resources += final_image;

			std::shared_ptr<Scene> scene = std::make_shared<Scene>(Scene::CI{
				.app = this,
				.name = "scene",
			});

			createScene(scene);
			scene->prepareForRendering();

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
			exec.getDebugRenderer()->setTargets(final_image, renderer.depth());


			PictureInPicture pip = PictureInPicture::CI{
				.app = this,
				.name = "PiP",
				.target = final_image,
				.sets_layouts = sets_layouts,
			};

			
			exec.init();

			struct InputState
			{

			};
			InputState input_state;

			for (int jid = 0; jid <= GLFW_JOYSTICK_LAST; ++jid)
			{
				if (glfwJoystickPresent(jid))
				{
					std::cout << "Joystick " << jid << ": " << glfwGetJoystickName(jid) << std::endl;
				}
			}

			KeyboardListener keyboard(*window);
			MouseListener mouse(*window);
			GamepadListener gamepad(*window, 0);

			const auto process_input = [&](InputState& state)
			{
				if (glfwGetKey(*window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
					glfwSetWindowShouldClose(*window, true);
			};

			double t = glfwGetTime(), dt = 0.0;
			size_t frame_index = 0;

			Camera camera(Camera::CreateInfo{
				.resolution = window->extent2D(),
			});

			FirstPersonCameraController camera_controller(FirstPersonCameraController::CreateInfo{
				.camera = &camera, 
				.keyboard = &keyboard,
				.mouse = &mouse,
				.gamepad = &gamepad,
			});

			while (!window->shouldClose())
			{
				{
					double new_t = glfwGetTime();
					dt = new_t - t;
					t = new_t;
				}
				window->pollEvents();

				const auto& imgui_io = ImGui::GetIO();
				
				if(!imgui_io.WantCaptureKeyboard)
				{
					keyboard.update();
				}
				if(!imgui_io.WantCaptureMouse)
				{
					mouse.update();
				}
				
				gamepad.update();
				process_input(input_state);
				camera_controller.updateCamera(dt);

				beginImGuiFrame();
				{
					ImGui::ShowDemoWindow();

					renderer.declareImGui();

					camera.declareImGui();

					window->declareImGui();

					pip.declareImGui();

					exec.getDebugRenderer()->declareImGui();
				}
				ImGui::EndFrame();

				if(mouse.getButton(1).justReleased())
				{
					pip.setPosition(mouse.getReleasedPos(1) / glm::vec2(window->extent2D().value().width, window->extent2D().value().height));
				}

				{
					scene->prepareForRendering();

					std::shared_ptr<UpdateContext> update_context = resources_manager.beginUpdateCycle();
					
					scene->updateResources(*update_context);
					exec.updateResources(*update_context);
					renderer.updateResources(*update_context);
					pip.updateResources(*update_context);
					script_resources.update(*update_context);
					
					resources_manager.finishUpdateCycle(update_context);
				}
				{
					exec.beginFrame();
					ExecutionThread * ptr_exec_thread = exec.beginCommandBuffer();
					ExecutionThread& exec_thread = *ptr_exec_thread;

					
					{
						UploadResources uploader(UploadResources::CI{
							.app = this,
						});
						exec_thread(uploader(UploadResources::UI{
							.holder = scene,
						}));
					}

					exec.bindSet(1, scene->set());
					renderer.execute(exec_thread, camera, t, dt, frame_index);

					pip.execute(exec_thread);

					exec.bindSet(1, nullptr);

					exec.renderDebugIFP();
					ImGui::Render();
					exec.preparePresentation(final_image);
					
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					exec.endCommandBufferAndSubmit(ptr_exec_thread);
					exec.present();


					++frame_index;
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
		bool vl = true;
#ifdef NDEBUG
		vl = false;
#endif
		vkl::RendererApp app(vl);
		app.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}