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
	static void LoadSponza(std::shared_ptr<Scene>& scene, int preset)
	{
		VkApplication* app = scene->application();
		std::shared_ptr<Scene::Node> root = scene->getRootNode();
		std::filesystem::path mtl = {};
		const bool sunlight = preset == 3;
		if (sunlight)
		{
			scene->setEnvironment(
				vec3(0.5, 0.8, 1) * 2,
				vec2(Radians(15.0f), Radians(60.0f)),
				Radians(0.5f),
				vec3(1.2, 1.1, 1) * 20
			);
		}
		if (preset == 1)
		{
			mtl = "assets:/models/sponza/caustics.mtl";
		}
		else if (preset == 2)
		{
			mtl = "assets:/models/sponza/metallic.mtl";
		}
		std::shared_ptr<NodeFromFile> sponza_node = std::make_shared<NodeFromFile>(NodeFromFile::CI{
			.app = app,
			.name = "Sponza",
			.matrix = DiagonalMatrix<3, 4>(0.01f),
			.path = "assets:/models/sponza/sponza.obj",
			.mtl_path = std::move(mtl),
			.synch = false,
		});
		root->addChild(sponza_node);

		if (!sunlight)
		{
			std::shared_ptr<Scene::Node> spotlights = std::make_shared<Scene::Node>(Scene::Node::CI{
				.name = "SpotLights",
				.matrix = TranslationMatrix(Vector3f(2, 1, 0)),
			});
			for (int i = 0; i < 3; ++i)
			{
				Vector3f color = Vector3f::Zero();
				color[i] = 10 * 5 * 2;
				std::shared_ptr<SpotLight> spot_light = std::make_shared<SpotLight>(SpotLight::CI{
					.app = app,
					.name = std::format("SpotLight_{}", i),
					.direction = Vector3f(0, 0, -1),
					.up = Vector3f(0, 1, 0),
					.emission = color,
					.attenuation = 1,
				});
				vec3 position = Vector3f::Zero();
				if (i == 1)
				{
					position += Vector3f(0.5, 0, 0);
				}
				else if (i == 2)
				{
					position += Vector3f(0.25, sqrt(3.0) / 2.0 * 0.5, 0);
				}
				std::shared_ptr<Scene::Node> spot_light_node = std::make_shared<Scene::Node>(Scene::Node::CI{
					.name = std::format("SpotLight_{}", i),
					.matrix = TranslationMatrix(position),
				});
				spot_light_node->light() = spot_light;
				spotlights->addChild(spot_light_node);
			}
			root->addChild(spotlights);
		}

		{
			std::shared_ptr<PBMaterial> material = std::make_shared<PBMaterial>(PBMaterial::CI{
				.app = app,
				.name = "mirror",
				.albedo = Vector3f::Constant(0.7).eval(),
				.metallic = 1.0f,
				.roughness = 0.0f,
				.cavity = 0,
			});

			std::shared_ptr<RigidMesh> mesh = RigidMesh::MakeCube(RigidMesh::CubeMakeInfo{
				.app = app,
			});

			std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
				.app = app,
				.name = "Cube",
				.mesh = mesh,
				.material = material,
			});

			std::shared_ptr<Scene::Node> model_node = std::make_shared<Scene::Node>(Scene::Node::CI{
				.name = "Mirror",
				.matrix = TranslationMatrix(Vector3f(4, 0.5, -2.5)) * ScalingMatrix(Vector3f(2, 1, 0.1)),
				.model = model,
			});
			root->addChild(model_node);
		}

		if(!sunlight)
		{
			std::shared_ptr<PointLight> pl = std::make_shared<PointLight>(PointLight::CI{
				.app = app,
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

		if (true)
		{
			std::shared_ptr<RigidMesh> mesh = RigidMesh::MakeSphere(RigidMesh::SphereMakeInfo{
				.app = app,
				.radius = 0.4,
			});

			const size_t n_m = 5;
			const size_t n_r = 5;

			const vec3 base(0, 0.27, 0);
			const vec3 dir_m(1, 0, 0);
			const vec3 dir_r(0, 0, 1);

			Matrix3x4f node_matrix = TranslationMatrix(vec3(-1, 0, -2)) * (DiagonalMatrix<3, 4>(0.2f));

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
						.app = app,
						.name = std::format("TestMaterial_{0:d}_{1:d}", i_m, i_r),
						.albedo = Vector3f::Constant(0.7).eval(),
						.metallic = metallic,
						.roughness = roughness,
						.cavity = 0,
					});

					std::shared_ptr<Model> model = std::make_shared<Model>(Model::CreateInfo{
						.app = app,
						.name = std::format("TestModel_{0:d}_{1:d}", i_m, i_r),
						.mesh = mesh,
						.material = material,
					});

					std::shared_ptr<Scene::Node> model_node = std::make_shared<Scene::Node>(Scene::Node::CI{
						.name = std::format("TestModelNode_{0:d}_{1:d}", i_m, i_r),
						.matrix = Matrix3x4f(TranslationMatrix(position)),
						.model = model,
					});
					test_material_node->addChild(model_node);
				}
			}
			root->addChild(test_material_node);
		}

		if (true)
		{
			std::shared_ptr<Scene::Node> in_spotlight = std::make_shared<Scene::Node>(Scene::Node::CI{
				.name = "In Spotlight",
				.matrix = TranslationMatrix(vec3(1.7, 0.25, -1.8)) * ScalingMatrix(vec3::Constant(0.25).eval()),
			});
			root->addChild(in_spotlight);

			in_spotlight->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Sphere",
				.xform = TranslationMatrix(vec3(0, 0, 0)),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Sphere,
				.subdivisions = uvec4(128, 256, 0, 0),
				
				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = false,
			}));

			in_spotlight->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Cube",
				.xform = TranslationMatrix(vec3(2, -0.5, 2)) * ScalingMatrix(vec3::Constant(1.5).eval()),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Cube,

				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = false,
			}));

			in_spotlight->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Tetrahedron",
				.xform = TranslationMatrix(vec3(3, -0.5, -1.5)) * 
						ScalingMatrix(vec3::Constant(1).eval()) * 
						MakeAffineTransform(Rotation3D(vec3(Radians(16.88f), Radians(52.5f), Radians(67.46f)))),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Tetrahedron,

				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = false,
			}));
		}
	}

	static void LoadCornellBox(std::shared_ptr<Scene>& scene, int preset_id)
	{
		struct Preset
		{
			bool metallic_walls;
			bool caustics;
			bool alternate_boxes;
		};
		std::array<Preset, 6> presets = {
			Preset{.metallic_walls = false, .caustics = false, .alternate_boxes = false},
			Preset{.metallic_walls = true, .caustics = false, .alternate_boxes = false},
			Preset{.metallic_walls = false, .caustics = true, .alternate_boxes = false},
			Preset{.metallic_walls = true, .caustics = true, .alternate_boxes = false},
			Preset{.metallic_walls = false, .caustics = false, .alternate_boxes = true},
			Preset{.metallic_walls = true, .caustics = false, .alternate_boxes = true},
		};
		Preset preset = presets[preset_id];
		VkApplication* app = scene->application();
		std::shared_ptr<Scene::Node> root = scene->getRootNode();
		
		std::shared_ptr<Scene::Node> box = std::make_shared<Scene::Node>(Scene::Node::CI{
			.name = "Cornell Box",
			.matrix = ScalingMatrix(Vector3f::Constant(2).eval()),
		});

		constexpr const float half_pi = std::numbers::pi_v<float> * 0.5f;

		root->addChild(box);

		vec3 gray	= vec3(0.7, 0.7, 0.7);
		vec3 green	= vec3(0.5, 1.0, 0.5);
		vec3 red	= vec3(1.0, 0.5, 0.5);
		vec3 orange = vec3(0.9, 0.6, 0.1);
		vec3 blue	= vec3(0.5, 0.5, 1.0);
		float red_metallic = preset.metallic_walls ? 0.9 : 0;
		float green_metallic = red_metallic;
		float red_roughness = preset.metallic_walls ? 0.1 : 1;
		float green_roughness = preset.metallic_walls ? 0.05 : 1;

		std::shared_ptr<RigidMesh> wall_mesh = RigidMesh::MakeSquare(RigidMesh::Square3DMakeInfo{
			.app = app,
			.name = "Square",
			.wireframe = false,
		});

		std::shared_ptr<RigidMesh> box_mesh = RigidMesh::MakeCube(RigidMesh::CubeMakeInfo{
			.app = app,
			.name = "Cube",
			.wireframe = false,
			.face_normal = true,
		});

		auto MakeMaterial = [&](std::string_view name, vec3 const& color, float roughness = 1, float metallic = 0)
		{
			return std::make_shared<PBMaterial>(PBMaterial::CI{
				.app = app,
				.name = std::string(name),
				.albedo = color,
				.metallic = metallic,
				.roughness = roughness,
				.is_dielectric = false,
				.sample_spectral = false,
			});
		};

		auto MakeWallModel = [&](std::string_view name, AffineXForm3Df const& xform, vec3 const& color, float roughness = 1, float metallic = 0)
		{
			auto material = MakeMaterial(std::format("{}.Material", name), color, roughness, metallic);
			return MakeModelNode(name, wall_mesh, material, xform);
		};

		auto MakeBoxModel = [&](std::string_view name, AffineXForm3Df const& xform, vec3 const& color, float roughness = 1, float metallic = 0)
		{
			auto material = MakeMaterial(std::format("{}.Material", name), color, roughness, metallic);
			return MakeModelNode(name, box_mesh, material, xform);
		};

		box->addChild(MakeWallModel(
			"Ceiling",
			MakeAffineTransform(Rotation3XYZ(Vector3f(half_pi, 0, 0))) * TranslationMatrix(Vector3f(0, 0, -0.5)),
			gray
		));

		box->addChild(MakeWallModel(
			"Ground",
			MakeAffineTransform(Rotation3XYZ(Vector3f(-half_pi, 0, 0))) * TranslationMatrix(Vector3f(0, 0, -0.5)),
			gray
		));


		box->addChild(MakeWallModel(
			"Back Wall",
			MakeAffineTransform(Rotation3XYZ(Vector3f(0, 0, 0))) * TranslationMatrix(Vector3f(0, 0, -0.5)),
			gray
		));

		box->addChild(MakeWallModel(
			"Red Wall",
			MakeAffineTransform(Rotation3XYZ(Vector3f(0, half_pi, 0))) * TranslationMatrix(Vector3f(0, 0, -0.5)),
			red, red_roughness, red_metallic
		));

		box->addChild(MakeWallModel(
			"Green Wall",
			MakeAffineTransform(Rotation3XYZ(Vector3f(0, -half_pi, 0))) * TranslationMatrix(Vector3f(0, 0, -0.5)),
			green, green_roughness, green_metallic
		));

		if (preset.caustics)
		{
			float scale = 0.15;
			bool spectral = false;
			box->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Sphere",
				.xform = TranslationMatrix(vec3(0.3, -0.2, 0)) * ScalingMatrix(vec3::Constant(scale).eval()),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Sphere,
				.subdivisions = uvec4(128, 256, 0, 0),

				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = spectral,
			}));

			box->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Cube",
				.xform = TranslationMatrix(vec3(0, -0.2, -0.3)) * ScalingMatrix(vec3::Constant(scale * 1.5).eval()),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Cube,

				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = spectral,
			}));

			box->addChild(MakeModelNode(BasicModelNodeCreateInfo{
				.app = app,
				.name = "Tetrahedron",
				.xform = TranslationMatrix(vec3(-0.3, -0.2, 0)) *
						ScalingMatrix(vec3::Constant(scale).eval()) *
						MakeAffineTransform(Rotation3D(vec3(Radians(0.0f), Radians(40.0f), Radians(0.0f)))) *
						MakeAffineTransform(Rotation3D(vec3(Radians(16.88f), Radians(52.5f), Radians(67.46f)))),

				.mesh_type = RigidMesh::RigidMeshMakeInfo::Type::Tetrahedron,

				.albedo = vec3(0, 0, 0),
				.roughness = 0,
				.metallic_or_eta = 1.33333,
				.is_dielectric = true,
				.sample_spectral = spectral,
			}));
		}
		else
		{
			box->addChild(MakeBoxModel(
				"Big Box",
				TranslationMatrix(Vector3f(-0.15, -0.2, -0.2)) * MakeAffineTransform(Rotation3Y(Radians(20.0f))) * ScalingMatrix(Vector3f(0.3, 0.6, 0.3)),
				blue, preset.alternate_boxes ? 0 : 1, preset.alternate_boxes ? 1 : 0
			));

			box->addChild(MakeBoxModel(
				"Small Box",
				TranslationMatrix(Vector3f(0.2, -0.35, 0.15)) * MakeAffineTransform(Rotation3Y(Radians(-20.0f))) * ScalingMatrix(Vector3f(0.3, 0.3, 0.3)),
				orange, preset.alternate_boxes ? 0.01 : 1, preset.alternate_boxes ? 0.1 : 0
			));
		}

		box->addChild(MakeLightNode(LightNodeCreateInfo{
			.app = app,
			.name = "Point Light",
			.xform = TranslationMatrix(Vector3f(0, 0.45, 0)),
			.type = LightType::POINT,
			.emission = vec3(1.5, 1.2, 0.8),
			.enable_shadow_map = true,
		}));
	}

	static void LoadTestScene(std::shared_ptr<Scene>& scene, int preset)
	{
		VkApplication * app = scene->application();
		scene->setAmbient(vec3::Zero());
		std::shared_ptr<Scene::Node> root = scene->getRootNode();


		if (Range32i(1, 4).contains(preset))
		{
			LoadSponza(scene, preset - 1);
		}
		else if (Range32i(5, 6).contains(preset))
		{
			LoadCornellBox(scene, preset - 5);
		}
	}


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

			features.features_12.uniformAndStorageBuffer8BitAccess = VK_TRUE;
			features.features2.features.shaderInt16 = VK_TRUE;
			features.features_11.storageBuffer16BitAccess = VK_TRUE;

			features.features2.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
			features.features_11.uniformAndStorageBuffer16BitAccess = VK_TRUE;

			features.shader_atomic_float_ext.shaderSharedFloat32AtomicAdd = VK_TRUE;
			features.shader_atomic_float_ext.shaderBufferFloat32AtomicAdd = VK_TRUE;

			features.ray_tracing_validation_nv.rayTracingValidation = VK_TRUE;
			//features.compute_shader_derivative_khr.computeDerivativeGroupQuads = VK_TRUE;
		}

		virtual std::set<std::string_view> getDeviceExtensions() override
		{
			std::set<std::string_view> res = AppWithImGui::getDeviceExtensions();
			res.insert(VK_NV_FILL_RECTANGLE_EXTENSION_NAME);
			res.insert(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
			res.insert(VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME);
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
			if (true)
			{
				slang_test = std::make_shared<ComputeCommand>(ComputeCommand::CI{
					.app = this,
					.name = "SlangTest",
					.shader_path = "ShaderLib:/Spectrum/Test.slang",
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

			ImGuiListSelection default_scene_selection = ImGuiListSelection::CI{
				.name = "Scene selection",
				.options = {
					ImGuiListSelection::Option{
						.name = "None",
					},
					ImGuiListSelection::Option{
						.name = "Sponza",
					},
					ImGuiListSelection::Option{
						.name = "Sponza with caustics",
					},
					ImGuiListSelection::Option{
						.name = "Sponza metallic",
					},
					ImGuiListSelection::Option{
						.name = "Sponza sunlight",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box metallic walls",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box caustics",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box caustics metallic walls",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box alternate cubes",
					},
					ImGuiListSelection::Option{
						.name = "Cornell Box alternate cubes metallic walls",
					},
				},
			};
			std::optional<uint> load_scene_index = {};
			
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

					if (!load_scene_index)
					{
						if (ImGui::Begin("LoadScene"))
						{
							default_scene_selection.declare();

							if (ImGui::Button("Load"))
							{
								load_scene_index = uint(default_scene_selection.index());
								LoadTestScene(scene, *load_scene_index);
							}
						}
						ImGui::End();
					}
					
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

					if (slang_test)
					{
						exec_thread(slang_test);
					}

					color_correction.execute(exec_thread);
					
					pip.execute(exec_thread);


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