#include "Model.hpp"

namespace vkl
{

	Model::Model(CreateInfo const& ci):
		Drawable(ci.app, ci.name),
		_mesh(ci.mesh)
	{
		createSet();
	}

	void Model::createSet()
	{
		using namespace std::containers_operators;
		ShaderBindings bindings;

		_mesh->writeBindings(bindings);

		_set = std::make_shared<DescriptorSetAndPool>(DescriptorSetAndPool::CreateInfo{
			.app = application(),
			.name = name() + ".set",
			.layout = setLayout(),
			.bindings = bindings,
		});
	}

	std::shared_ptr<DescriptorSetAndPool> Model::setAndPool()
	{
		return _set;
	}

	ResourcesToDeclare Model::getResourcesToDeclare()
	{
		ResourcesToDeclare res = _mesh->getResourcesToDeclare();
		res.sets.push_back(_set);
		return res;
	}

	ResourcesToUpload Model::getResourcesToUpload()
	{
		ResourcesToUpload res = _mesh->getResourcesToUpload();

		return res;
	}

	void Model::notifyDataIsUploaded()
	{
		_mesh->notifyDataIsUploaded();
	}

	VertexInputDescription Model::vertexInputDesc() 
	{
		return _mesh->vertexInputDesc();
	}

	VertexInputDescription Model::vertexInputDescStatic()
	{
		VertexInputDescription res = RigidMesh::vertexInputDescStatic();

		return res;
	}

	std::shared_ptr<DescriptorSetLayout> Model::setLayout()
	{
		if (!_set_layout)
		{
			_set_layout = setLayout(SetLayoutOptions{
				.app = application(),
			});
		}
		return _set_layout;
	}


	std::shared_ptr<DescriptorSetLayout> Model::setLayout(SetLayoutOptions const& options)
	{
		struct CachedSetLayout
		{
			SetLayoutOptions options;
			std::shared_ptr<DescriptorSetLayout> layout;
		};

		static std::vector<CachedSetLayout> _cached;

		auto findSetLayoutIFP = [](SetLayoutOptions const& options) -> std::shared_ptr<DescriptorSetLayout>
		{
			for (size_t i = 0; i < _cached.size(); ++i)
			{
				if (_cached[i].options == options)
				{
					return _cached[i].layout;
				}
			}
			return nullptr;
		};

		auto cacheSetLayout = [&findSetLayoutIFP](SetLayoutOptions const& options, std::shared_ptr<DescriptorSetLayout> layout)
		{
			assert(!findSetLayoutIFP(options));
			_cached.push_back(CachedSetLayout{
				.options = options,
				.layout = layout,
			});
		};

		std::shared_ptr<DescriptorSetLayout> res = findSetLayoutIFP(options);

		if (!res)
		{
			using namespace std::containers_operators;
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			std::vector<DescriptorSetLayout::BindingMeta> metas;

			bindings += VkDescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			};
			metas += DescriptorSetLayout::BindingMeta{
				.name = "MeshHeader",
				.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			bindings += VkDescriptorSetLayoutBinding{
				.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr,
			};
			metas += DescriptorSetLayout::BindingMeta{
				.name = "MeshVertices",
					.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			bindings += VkDescriptorSetLayoutBinding{
				.binding = 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			};
			metas += DescriptorSetLayout::BindingMeta{
				.name = "MeshIndices32",
				.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			};

			VkDescriptorSetLayoutCreateFlags flags = 0;
			VkDescriptorBindingFlags binding_flags = 0;
			if (options.app->descriptorBindingGlobalOptions().use_push_descriptors)
			{
				flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			}
			else
			{
				flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
				binding_flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
			}

			res = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = options.app, 
				.name = "Model::DescriptorSetLayout",
				.flags = flags,
				.bindings = bindings,
				.metas = metas,
				.binding_flags = binding_flags,
			});
			cacheSetLayout(options, res);
		}
		return res;
	}

	void Model::recordBindAndDraw(ExecutionContext& ctx)
	{
		_mesh->recordBindAndDraw(*ctx.getCommandBuffer());
	}

	void Model::recordSynchForDraw(SynchronizationHelper& synch, std::shared_ptr<Pipeline> const& pipeline)
	{
		_mesh->recordSynchForDraw(synch, pipeline);
	}
}