
#include <ShaderLib/common.slang>
#include <ShaderLib/Vulkan/Indirect.h>

#define BIND_SCENE 1
#include <ShaderLib/Rendering/Scene/Scene.slang>

layout(SHADER_DESCRIPTOR_BINDING + 0) RWStructuredBuffer<VkDrawIndirectCommand> vk_draw_list;

layout(SHADER_DESCRIPTOR_BINDING + 1) RWStructuredBuffer<uint> index_buffer;

layout(SHADER_DESCRIPTOR_BINDING + 2) RWStructuredBuffer<uint> atomic_counter;

struct PushConstant
{
	uint num_objects;
}

[vk::push_constant]
uniform PushConstant _pc;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(uvec3 global_index : SV_DispatchThreadID)
{
	const uint gid = global_index.x;
	if(gid < _pc.num_objects)
	{
		let scene = BoundScene();
		let obj = SceneObjectsTable[gid];
		bool emit = (obj.flags & SCENE_OBJECT_FLAG_VISIBLE_BIT) != 0;
		const uint index = gid;//atomicAdd(atomic_counter.counter, 1);
		VkDrawIndirectCommand vk_draw;
		if(emit)
		{
			const uint mesh_index = obj.mesh_id;
			let mesh = BoundScene::MeshReference(mesh_index);
			const uint mesh_num_indices = mesh.getHeader().num_indices;
			
			index_buffer[index] = gid;

			vk_draw.vertexCount = mesh_num_indices;
			vk_draw.instanceCount = 1;
			vk_draw.firstVertex = 0;
			vk_draw.firstInstance = 0;
		}
		else
		{
			vk_draw.vertexCount = 0;
			vk_draw.instanceCount = 0;
			vk_draw.firstVertex = 0;
			vk_draw.firstInstance = 0;
		}
		vk_draw_list[index] = vk_draw;
	}
}