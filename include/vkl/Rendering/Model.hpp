#pragma once

#include <vkl/Rendering/Drawable.hpp>
#include "Mesh.hpp"
#include <vkl/Execution/DescriptorSetsManager.hpp>
#include <vkl/Execution/ResourcesHolder.hpp>
#include <vkl/Rendering/Material.hpp>
#include <vkl/GUI/Context.hpp>

namespace vkl
{
	class Model : public VkObject, public Drawable
	{
	public:

		template <Mesh::Type mesh, Material::Type material>
		static constexpr uint32_t MakeTypeT()
		{
			return static_cast<uint32_t>(mesh) | (static_cast<uint32_t>(material) << 16);
		}

		static constexpr uint32_t MakeType(Mesh::Type mesh, Material::Type material)
		{
			return static_cast<uint32_t>(mesh) | (static_cast<uint32_t>(material) << 16);
		}

		static constexpr Mesh::Type ExtractMeshType(uint32_t model_type)
		{
			return static_cast<Mesh::Type>(model_type & 0xffff);
		}

		static constexpr Material::Type ExtractMaterialType(uint32_t model_type)
		{
			return static_cast<Material::Type>(model_type >> 16);
		}

	protected:

		// A combination of Mesh and Material type
		uint32_t _type = 0;
		
		std::filesystem::path _mesh_path = {};
		std::shared_ptr<Mesh> _mesh = nullptr;

		std::shared_ptr<Material> _material = nullptr;

		std::shared_ptr<DescriptorSetLayout> _set_layout = nullptr;

		std::shared_ptr<DescriptorSetAndPool> _set;

		bool _synch = true;

		void createSet();


		static constexpr uint32_t mesh_binding_offset = 0;
		static constexpr uint32_t material_binding_offset = 8;


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			// mesh_path xor mesh
			std::filesystem::path mesh_path = {};
			std::shared_ptr<Mesh> mesh = nullptr;
			
			std::shared_ptr<Material> material = nullptr;

			bool synch = true;
		};
		using CI = CreateInfo;

		Model(CreateInfo const& ci);

		virtual ~Model() override;

		const std::shared_ptr<Mesh>& mesh()const
		{
			return _mesh;
		}

		const std::shared_ptr<Material>& material()const
		{
			return _material;
		}

		constexpr uint32_t type()const
		{
			return _type;
		}

		//virtual void recordSynchForDraw(SynchronizationHelper& synch, std::shared_ptr<Pipeline> const& pipeline) override final;

		virtual void updateResources(UpdateContext & ctx);

		virtual VertexInputDescription vertexInputDesc() override final;

		static VertexInputDescription vertexInputDescStatic();

		//virtual void recordBindAndDraw(ExecutionContext& ctx) override final;

		virtual void fillVertexDrawCallInfo(VertexDrawCallInfo& vr) override;

		virtual std::shared_ptr<DescriptorSetLayout> setLayout();


		struct SetLayoutOptions
		{
			uint32_t type = 0;
			bool bind_mesh = true;
			bool bind_material = true;

			constexpr bool operator==(SetLayoutOptions const& o) const
			{
				return (type == o.type)
					&& (bind_mesh == o.bind_mesh) 
					&& (bind_material == o.bind_material);
			}
		};

		using ModelSetLayoutCache = DescriptorSetLayoutCacheImpl<SetLayoutOptions>;

		static std::shared_ptr<DescriptorSetLayout> setLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetAndPool> setAndPool();

		virtual bool isReadyToDraw() const;

		virtual void declareGui(GUI::Context & ctx);




		
		struct LoadInfo
		{
			VkApplication * app = nullptr;
			std::filesystem::path path = {};
			std::filesystem::path mtl_path = {};
			bool synch = true;
		};

		static std::vector<std::shared_ptr<Model>> loadModelsFromObj(LoadInfo const& info);


	};
}