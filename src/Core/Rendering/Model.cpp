#include "Model.hpp"
#include <tinyobj/tiny_obj_loader.h>
#include <Core/Execution/SamplerLibrary.hpp>

#include <Core/Rendering/TextureFromFile.hpp>

#include <unordered_map>
#include <functional>

namespace vkl
{

	Model::Model(CreateInfo const& ci):
		VkObject(ci.app, ci.name),
		Drawable(),
		_mesh(ci.mesh),
		_material(ci.material),
		_synch(ci.synch)
	{
		assert(!!_mesh xor (!ci.mesh_path.empty()));
		_type = MakeType(_mesh->type(), _material->type());
		createSet();

		if (_mesh)
		{
			_mesh->registerToDescriptorSet(_set, mesh_binding_offset, 0);
		}

		if (_material)
		{
			_material->installResourceUpdateCallbacks(_set, material_binding_offset);
		}
	}

	Model::~Model()
	{
		if (_mesh)
		{
			_mesh->unRegistgerFromDescriptorSet(_set);
		}
		if (_material)
		{
			_material->removeResourceUpdateCallbacks(_set);
		}
	}

	void Model::createSet()
	{
		using namespace std::containers_append_operators;
		ShaderBindings bindings;

		bindings += _mesh->getShaderBindings(mesh_binding_offset);

		if (!!_material)
		{
			bindings += _material->getShaderBindings(material_binding_offset);
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

	void Model::updateResources(UpdateContext& ctx)
	{
		if (_mesh)
		{
			_mesh->updateResources(ctx);
		}
		if (_material)
		{
			_material->updateResources(ctx);
		}

		_set->updateResources(ctx);
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
				.type = _type,
				.bind_mesh = true,
				.bind_material = !!_material,
			});
		}
		return _set_layout;
	}


	std::shared_ptr<DescriptorSetLayout> Model::setLayout(VkApplication * app, SetLayoutOptions const& options)
	{
		std::shared_ptr<DescriptorSetLayoutCache> gen_cache = app->getDescSetLayoutCacheOrEmplace(app->descriptorBindingGlobalOptions().set_bindings[uint32_t(DescriptorSetName::invocation)].set, []()
		{
			return std::make_shared<ModelSetLayoutCache>();
		});
		ModelSetLayoutCache * cache = dynamic_cast<ModelSetLayoutCache*>(gen_cache.get());
		assert(!!cache);

		std::shared_ptr<DescriptorSetLayout> res = cache->findOrEmplace(options, [app, &options]()
		{
			using namespace std::containers_append_operators;

			std::vector<DescriptorSetLayout::Binding> bindings;

			if (options.bind_mesh)
			{
				Mesh::Type mesh_type = ExtractMeshType(options.type);
				switch (mesh_type)
				{
				case Mesh::Type::Rigid:
					bindings += RigidMesh::getSetLayoutBindingsStatic(mesh_binding_offset);
					break;
				default:
					assert(false);
					break;
				}
			}

			if (options.bind_material)
			{
				Material::Type material_type = ExtractMaterialType(options.type);
				switch (material_type)
				{
				case Material::Type::PhysicallyBased:
					bindings += Material::getSetLayoutBindings(Material::Type::PhysicallyBased, material_binding_offset);
					break;
				default:
					assert(false);
					break;
				}
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
				binding_flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			}

			std::shared_ptr<DescriptorSetLayout> res = std::make_shared<DescriptorSetLayout>(DescriptorSetLayout::CI{
				.app = app,
				.name = "Model::DescriptorSetLayout",
				.flags = flags,
				.bindings = bindings,
				.binding_flags = binding_flags,
			});
			return res;
		});

		return res;
	}

	void Model::fillVertexDrawCallInfo(VertexDrawCallInfo& vr)
	{
		if (_mesh)
		{
			_mesh->fillVertexDrawCallInfo(vr);
		}
	}

	bool Model::isReadyToDraw()const
	{
		bool res = true;
		if (_mesh)
		{
			res = _mesh->isReadyToDraw();
		}
		return res;
	}

	void Model::declareGui(GuiContext& ctx)
	{
		ImGui::PushID(name().c_str());
		ImGui::Text(name().c_str());

		if (ImGui::CollapsingHeader("Mesh"))
		{
			if (_mesh)
			{
				_mesh->declareGui(ctx);
			}
			else
			{
				ImGui::Text("None");
			}
		}

		if (ImGui::CollapsingHeader("Material"))
		{
			if (_material)
			{
				_material->declareGui(ctx);
			}
			else
			{
				ImGui::Text("None");
			}
		}

		ImGui::PopID();
	}












	std::vector<std::shared_ptr<Model>> Model::loadModelsFromObj(LoadInfo const& info)
	{
		std::vector<std::shared_ptr<Model>> res;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		std::string warn, err;

		const std::filesystem::path mtl_path = [&info]() {
			return info.path.parent_path();
		}();
		const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, info.path.string().c_str(), mtl_path.string().c_str());

		if (!warn.empty())
		{
			std::cout << "Warning: " << warn << std::endl;
		}
		if (!err.empty())
		{
			std::cerr << "Error: " << err << std::endl;
		}

		TextureFileCache & texture_file_cache = info.app->textureFileCache();

		if (ret)
		{
			std::vector<std::shared_ptr<Material>> my_materials(materials.size());
			for (size_t m = 0; m < materials.size(); ++m)
			{
				const tinyobj::material_t & tm = materials[m];

				std::shared_ptr<Sampler> sampler = info.app->getSamplerLibrary().getSampler({
					.filter = VK_FILTER_LINEAR,
					.address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.max_anisotropy = info.app->deviceProperties().props.limits.maxSamplerAnisotropy,
				});

				std::filesystem::path albedo_path = tm.diffuse_texname.empty() ? std::filesystem::path() : mtl_path / tm.diffuse_texname;
				std::filesystem::path normal_path = tm.normal_texname.empty() ? std::filesystem::path() : mtl_path / tm.normal_texname;

				std::shared_ptr<Texture> albedo_texture = texture_file_cache.getTexture(albedo_path); 
				std::shared_ptr<Texture> normal_texture = texture_file_cache.getTexture(normal_path);
				
				my_materials[m] = std::make_shared<PBMaterial>(PBMaterial::CI{
					.app = info.app,
					.name = tm.name,
					.albedo = glm::vec3(tm.diffuse[0], tm.diffuse[1], tm.diffuse[2]),
					.sampler = sampler,
					.albedo_texture = albedo_texture,
					.normal_texture = normal_texture,
					.synch = info.synch,
				});
			}

			for (size_t s = 0; s < shapes.size(); ++s)
			{
				const tinyobj::shape_t & shape = shapes[s];
				const tinyobj::mesh_t & tm = shape.mesh;

				// TODO Better
				const size_t material_index = tm.material_ids[0];
				std::shared_ptr<Material> material = my_materials[material_index];

				const auto readVec3 = [](std::vector<tinyobj::real_t> const& vec, size_t i)
				{
					return glm::vec3(vec[3 * i + 0], vec[3 * i + 1], vec[3 * i + 2]);
				};
				const auto readVec2 = [](std::vector<tinyobj::real_t> const& vec, size_t i)
				{
					return glm::vec2(vec[2 * i + 0], vec[2 * i + 1]);
				};

				struct TinyIndexHasher
				{
					size_t operator()(tinyobj::index_t const index)const
					{
						std::hash<int> hi;
						std::hash<size_t> hs;
						return hi(index.vertex_index) ^ hi(index.normal_index) ^ hi(index.texcoord_index);
					}
				};
				struct TinyIndexEqual
				{
					constexpr bool operator()(tinyobj::index_t const& lhs, tinyobj::index_t const& rhs) const
					{
						return (lhs.vertex_index == rhs.vertex_index) && (lhs.normal_index == rhs.normal_index) && (lhs.texcoord_index == rhs.texcoord_index);
					}
				};

				std::unordered_map<tinyobj::index_t, size_t, TinyIndexHasher, TinyIndexEqual> vertices_map;
				std::vector<Vertex> vertices;
				std::vector<uint32_t> indices;

				for (size_t i = 0; i < tm.indices.size(); ++i)
				{
					tinyobj::index_t tiny_index = tm.indices[i];
					if(!vertices_map.contains(tiny_index))
					{
						Vertex v{
							.position = Vector4f(readVec3(attrib.vertices, tiny_index.vertex_index), 1),
							.normal = Vector4f(readVec3(attrib.normals, tiny_index.normal_index), 0),
							.uv = Vector4f(readVec2(attrib.texcoords, tiny_index.texcoord_index), 0, 0),
						};
						// .obj flips the texture
						v.uv.y = 1.0f - v.uv.y;
						vertices_map[tiny_index] = vertices.size();
						vertices.push_back(v);
					}

					indices.push_back(vertices_map.at(tiny_index));
				}

				std::shared_ptr<RigidMesh> mesh = std::make_shared<RigidMesh>(RigidMesh::CI{
					.app = info.app,
					.name = shape.name,
					.vertices = vertices,
					.indices = indices,
					.auto_compute_tangents = true,
					.synch = info.synch,
				});
				
				std::shared_ptr<Model> model = std::make_shared<Model>(Model::CI{
					.app = info.app,
					.name = shape.name,
					.mesh = mesh,
					.material = material,
					.synch = info.synch,
				});

				res.push_back(model);
			}
			
		}
		else
		{
			
		}
		
		



		return res;
	}
}