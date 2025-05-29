
#define BIND_SINGLE_MATERIAL 0
#define BIND_SCENE 1
#define BIND_RENDERER_SET 1

#include "shading.slang"

#include <ShaderLib/Rendering/Materials/PBMaterial.slang>

#include "IndirectCommon.slang"

#define LIGHTS_ACCESS ROM
#include <ShaderLib/Rendering/Scene/Scene.slang>

layout(location = 0) in nointerpolation uvec4 v_flat; 
layout(location = 1) in vec3 v_w_position;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_w_normal;
layout(location = 4) in vec3 v_w_tangent;

layout(location = 0) out vec4 o_color;

[shader("pixel")]
void main()
{
	const uint draw_id = v_flat.x;
	const uint object_id = indirect_model_indices[draw_id];
	let object_ref = SceneObjectsTable[object_id];
	const uint material_id = object_ref.material_id;

	let scene = BoundScene();

	let material = scene.readMaterial(material_id, v_uv, true, QuadDiffTextureSampling());

	const vec2 uv = v_uv;
	const vec3 position = v_w_position; 
	const vec3 a_normal = normalize(v_w_normal);
	const vec3 normal = a_normal;
	vec3 tangent = SafeNormalize(v_w_tangent);
	tangent = SafeNormalize(tangent - dot(tangent, normal) * normal);
	const vec3 bi_tangent = cross(tangent, normal);

	SurfaceGeometryInfo geom;
	geom.position = position;
	geom.vertex_shading_normal = normal;

	geom.geometry_normal = normalize(cross(ddy(geom.position), ddx(geom.position)));

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_VERTEX
	geom.shading_normal = geom.vertex_shading_normal;
#else
	geom.shading_normal = geom.geometry_normal;
#endif

	geom.vertex_shading_tangent = tangent;

#if SHADING_ENABLE_OPACITY_TEST
	if(scene.testOpacity(material.alpha))
	{
		discard;
	}
#endif

	ApplyMaterialNormalIFN(geom, material);

	let camera = MakeMatrixCamera(renderer_ubo.camera);
	const vec3 camera_position = camera.getPosition();
	const vec3 wo = normalize(camera_position - position);

	var res = Shade(scene, geom, material);
	
	res.L += material.albedo * scene.getUBO().ambient;

	o_color = vec4(res.L, 1);
}