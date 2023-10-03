#pragma once

#include <Core/Rendering/Drawable.hpp>
#include "Mesh.hpp"
#include <Core/Execution/DescriptorSetsManager.hpp>
#include <Core/Execution/ResourcesHolder.hpp>

namespace vkl
{
	class Model : public Drawable, public ResourcesHolder
	{
	protected:
		
		std::shared_ptr<Mesh> _mesh = nullptr;

		std::shared_ptr<DescriptorSetLayout> _set_layout = nullptr;

		std::shared_ptr<DescriptorSetAndPool> _set;

		void createSet();


	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			std::shared_ptr<Mesh> mesh = nullptr;
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
			constexpr bool operator==(SetLayoutOptions const& o) const
			{
				return true;
			}
		};

		using ModelSetLayoutCache = DescriptorSetLayoutCacheImpl<SetLayoutOptions>;

		static std::shared_ptr<DescriptorSetLayout> setLayout(VkApplication * app, SetLayoutOptions const& options);

		virtual std::shared_ptr<DescriptorSetAndPool> setAndPool();


	};
}