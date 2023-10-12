#include "Model.hpp"

namespace vkl
{

	Model::Model(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		Drawable(),
		ResourcesHolder(),
		_mesh(ci.mesh),
		_material(ci.material)
	{
		assert(!!_mesh xor (!ci.mesh_path.empty()));
		createSet();
	}

	void Model::createSet()
	{
		using namespace std::containers_operators;
		ShaderBindings bindings;

		bindings += _mesh->getShaderBindings(mesh_binding_offset);

		if (!!_material)
		{
			bindings += _material->getShaderBindings(material_binding_offset);
		}
		else
		{

		}

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
		ResourcesToDeclare res;
		res += _mesh->getResourcesToDeclare();
		if (_material)
		{
			res += _material->getResourcesToDeclare();
		}
		res += _set;
		return res;
	}

	ResourcesToUpload Model::getResourcesToUpload()
	{
		ResourcesToUpload res;
		if(_mesh->getStatus().device_up_to_date == false) 
		{
			res += _mesh->getResourcesToUpload();
		}

		if (_material)
		{
			res += _material->getResourcesToUpload();
		}

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
		VertexInputDescription res = RigidMesh::vertexInputDescFullVertex();

		return res;
	}

	std::shared_ptr<DescriptorSetLayout> Model::setLayout()
	{
		if (!_set_layout)
		{
			_set_layout = setLayout(application(), SetLayoutOptions{
				.bind_mesh = true,
				.bind_material = !!_material,
			});
		}
		return _set_layout;
	}


	std::shared_ptr<DescriptorSetLayout> Model::setLayout(VkApplication * app, SetLayoutOptions const& options)
	{
		std::shared_ptr<DescriptorSetLayoutCache> & gen_cache = app->getDescSetLayoutCache(app->descriptorBindingGlobalOptions().set_bindings[uint32_t(DescriptorSetName::object)].set);

		if (!gen_cache)
		{
			gen_cache = std::make_shared<ModelSetLayoutCache>();
		}

		ModelSetLayoutCache * cache = dynamic_cast<ModelSetLayoutCache*>(gen_cache.get());

		std::shared_ptr<DescriptorSetLayout> res = cache->findIFP(options);

		if (!res)
		{
			using namespace std::containers_operators;

			std::vector<DescriptorSetLayout::Binding> bindings;

			if(options.bind_mesh)
			{
				bindings += RigidMesh::getSetLayoutBindingsStatic(mesh_binding_offset);
			}

			if (options.bind_material)
			{
				bindings += Material::getSetLayoutBindings(Material::Type::PhysicallyBased, material_binding_offset);
			}

			VkDescriptorSetLayoutCreateFlags flags = 0;
			VkDescriptorBindingFlags binding_flags = 0;
			if (app->descriptorBindingGlobalOptions().use_push_descriptors)
			{
				flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			}
			else
			{
				flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
				binding_flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
			}

			res = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = app, 
				.name = "Model::DescriptorSetLayout",
				.flags = flags,
				.bindings = bindings,
				.binding_flags = binding_flags,
			});
			cache->recordValue(options, res);
		}
		return res;
	}

	void Model::recordBindAndDraw(ExecutionContext& ctx)
	{
		_mesh->recordBindAndDraw(ctx);
	}

	void Model::recordSynchForDraw(SynchronizationHelper& synch, std::shared_ptr<Pipeline> const& pipeline)
	{
		_mesh->recordSynchForDraw(synch, pipeline);
	}
}