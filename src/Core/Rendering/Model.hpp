#pragma once

#include <Core/Rendering/Drawable.hpp>
#include "Mesh.hpp"
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ResourcesHolder.hpp>
#include <Core/Rendering/Material.hpp>

namespace vkl
{
	class Model : public VkObject, public Drawable, public ResourcesHolder
	{
	protected:
		
		std::filesystem::path _mesh_path = {};
		std::shared_ptr<Mesh> _mesh = nullptr;

		std::shared_ptr<Material> _material = nullptr;

		std::shared_ptr<DescriptorSetLayout> _set_layout = nullptr;

		std::shared_ptr<DescriptorSetAndPool> _set;

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
		};
		using CI = CreateInfo;

		Model(CreateInfo const& ci);

		std::shared_ptr<Mesh> mesh()const
		{
			return _mesh;
		}

		virtual void recordSynchForDraw(SynchronizationHelper& synch, std::shared_ptr<Pipeline> const& pipeline) override final;

		virtual ResourcesToDeclare getResourcesToDeclare() override final;

		virtual ResourcesToUpload getResourcesToUpload() override final;

		virtual void notifyDataIsUploaded() override final;

		virtual VertexInputDescription vertexInputDesc() override final;

		static VertexInputDescription vertexInputDescStatic();

		virtual void recordBindAndDraw(ExecutionContext& ctx) override final;

		virtual std::shared_ptr<DescriptorSetLayout> setLayout() override final;


		struct SetLayoutOptions
		{
			bool bind_mesh = true;
			bool bind_material = true;

			constexpr bool operator==(SetLayoutOptions const& o) const
			{
				return (bind_mesh == o.bind_mesh) 
					&& (bind_material == o.bind_material);
			}
		};

		using ModelSetLayoutCache = DescriptorSetLayoutCacheImpl<SetLayoutOptions>;

		static std::shared_ptr<DescriptorSetLayout> setLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetAndPool> setAndPool();


	};
}