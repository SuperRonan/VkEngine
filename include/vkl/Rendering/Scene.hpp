#pragma once

#include <vkl/App/VkApplication.hpp>

#include <vkl/Maths/Types.hpp>
#include <vkl/Maths/AffineXForm.hpp>


#include <vkl/VkObjects/Buffer.hpp>
#include <vkl/VkObjects/ImageView.hpp>
#include <vkl/VkObjects/AccelerationStructure.hpp>

#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/Execution/GrowableBuffer.hpp>
#include <vkl/Execution/HostManagedBuffer.hpp>

#include <vkl/Rendering/SceneNode.hpp>
#include <vkl/Rendering/DirectedAcyclicGraph.hpp>
#include <vkl/Rendering/Model.hpp>
#include <vkl/Rendering/Light.hpp>

#include <vkl/Utils/UniqueIndexAllocator.hpp>
#include <vkl/Utils/IndexBool.hpp>

#include <vkl/VkObjects/TopLevelAccelerationStructure.hpp>
#include <vkl/Commands/AccelerationStructureCommands.hpp>

#include <unordered_map>

#include <cassert>

#include <that/utils/array.hpp>

namespace vkl
{
	namespace impl
	{
		struct SceneHelper;
	}
	class Scene : public VkObject
	{
	
	public:

		using Vec3 = Vector3f;
		using Vec4 = Vector4f;
		using Mat3 = Matrix3f;
		using Mat4 = Matrix4f;
		using Mat3x4 = Matrix3x4f;

		using Node = SceneNode;
		using DAG = DirectedAcyclicGraph;

		struct LightInstanceSpecificData
		{
			virtual ~LightInstanceSpecificData() = default;
		};

	protected:
		friend struct impl::SceneHelper;

		std::shared_ptr<DirectedAcyclicGraph> _tree;

		uint32_t _lights_bindings_base;
		uint32_t _objects_binding_base;
		uint32_t _mesh_bindings_base;
		uint32_t _material_bindings_base;
		uint32_t _textures_bindings_base;
		uint32_t _xforms_bindings_base;
		uint32_t _tlas_binding_base;

		std::shared_ptr<DescriptorSetLayout> _set_layout;
		std::shared_ptr<DescriptorSetAndPool> _set;

		std::unique_ptr<impl::SceneHelper> _internal;

		struct MaterialReference
		{
			using Id = uint16_t;
			using Ids = std::array<Id, Material::MAX_TEXTURE_COUNT>;
			static constexpr const Ids DefaultIds = that::MakeUniformArray<Material::MAX_TEXTURE_COUNT>(Id(-1));
			Ids ids = DefaultIds;
		};
		std::shared_ptr<HostManagedBuffer> _material_ref_buffer;

		struct ModelReference
		{
			uint32_t mesh_id;
			uint32_t material_id;
			uint32_t xform_id;
			uint32_t flags;
		};
		std::shared_ptr<HostManagedBuffer> _model_references_buffer;

		std::shared_ptr<HostManagedBuffer> _xforms_buffer;
		std::shared_ptr<Buffer> _prev_xforms_buffer;
		BufferAndRange _xforms_segment;
		BufferAndRange _prev_xforms_segment;

		struct UBO
		{	
			uint32_t num_lights;
			uint32_t num_objects;
			uint32_t num_mesh;
			uint32_t num_materials;

			uint32_t num_textures;
			uint32_t flags;

			ubo_vec3 ambient;
			
			ubo_vec3 sky;

			ubo_vec3 solar_direction;
			float solar_disk_cosine;

			ubo_vec3 solar_disk_emission;
			float solar_disk_angle;
			
			ubo_vec3 center;
			float radius;
		};

		AABB3f _aabb = {};

		vec3 _ambient = vec3::Constant(0.1f);
		vec3 _uniform_sky = vec3::Constant(0);
		float _uniform_sky_brightness = 1;

		vec2 _solar_disk_direction = vec2(0, 0);
		float _solar_disk_angle = Radians(0.5f); // Same as the sun viewed from earth
		vec3 _solar_disk_emission = vec3::Zero();
		uint8_t _solar_disk_emission_options = 0;

		float _radius = 1;


		std::shared_ptr<Buffer> _ubo_buffer;

		// Per frame number of lights
		uint32_t _num_lights;
		std::shared_ptr<HostManagedBuffer> _lights_buffer;
		UniqueIndexAllocator _unique_light_index_pool;
		UniqueIndexAllocator _light_depth_map_2D_index_pool, _light_depth_map_cube_index_pool;

		
		struct LightInstanceData
		{
			uint32_t unique_id;
			std::shared_ptr<ImageView> depth_view;
			std::unique_ptr<LightInstanceSpecificData> specific_data = nullptr;
			uint32_t depth_texture_unique_id;
			uint32_t frame_light_id;
			uint32_t flags;
		};
		std::unordered_map<DAG::RobustNodePath, LightInstanceData> _unique_light_instances;

		uint32_t _light_resolution = 1024 * 2;
		VkSampleCountFlagBits _light_depth_samples = VK_SAMPLE_COUNT_1_BIT;
		VkFormat _light_depth_format = VK_FORMAT_D32_SFLOAT;



		bool _maintain_rt = false;
		std::shared_ptr<TLAS> _tlas = nullptr;
		std::shared_ptr<BuildAccelerationStructureCommand> _build_tlas;
		BuildAccelerationStructureCommand::BuildInfo _tlas_build_info;
		
		
		UBO getUBO() const;

		void createInternalBuffers();

		void createSet();

		void fillLightsBuffer();

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
		};
		using CI = CreateInfo;

		Scene(CreateInfo const& ci);

		virtual ~Scene() override;

		const std::shared_ptr<DirectedAcyclicGraph>& getTree()const
		{
			return _tree;
		}

		const std::shared_ptr<Node>& getRootNode() const
		{
			return _tree->root();
		}

		struct SetLayoutOptions
		{
			
			constexpr bool operator==(const SetLayoutOptions& o) const
			{
				return true;
			}
		};

		// Call before updating resources
		void updateInternal();

		virtual void updateResources(UpdateContext & ctx);

		virtual void prepareForRendering(ExecutionRecorder & exec);

		virtual void buildTLAS(ExecutionRecorder & exec);

		//static std::shared_ptr<DescriptorSetLayout> SetLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();

		std::shared_ptr<DescriptorSetAndPool> set();

		uint32_t objectCount()const;

		VkFormat lightDepthFormat() const
		{
			return _light_depth_format;
		}

		VkSampleCountFlagBits lightDepthSamples() const
		{
			return _light_depth_samples;
		}

		void setMaintainRT(bool maintain_rt);

		void setAmbient(vec3 a)
		{
			_ambient = a;
		}

		vec3 getAmbient() const
		{
			return _ambient;
		}

		void setEnvironment(vec3 intensity, vec2 solar_direction, float solar_angle, vec3 solar_emission)
		{
			_uniform_sky = intensity;
			_uniform_sky_brightness = 1;
			_solar_disk_direction = solar_direction;
			_solar_disk_angle = solar_angle;
			_solar_disk_emission = solar_emission;
			_solar_disk_emission_options = 0;
		}

		void setEnvironment(vec3 intensity, vec2 solar_direction, float solar_angle, float solar_temperature, float solar_intensity)
		{
			_uniform_sky = intensity;
			_uniform_sky_brightness = 1;
			_solar_disk_direction = solar_direction;
			_solar_disk_angle = solar_angle;
			_solar_disk_emission = vec3(solar_temperature, solar_intensity, 0);
			_solar_disk_emission_options = BlackBodyEmission(1);
		}

		friend class SceneUserInterface;
		friend class SimpleRenderer;
	};
}