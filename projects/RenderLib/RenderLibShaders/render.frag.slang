
#define BIND_SINGLE_MATERIAL 1
#define BIND_SCENE 1
#define BIND_RENDERER_SET 1

#include "shading.slang"

#define LIGHTS_ACCESS ROM
#include <ShaderLib/Rendering/Scene/Scene.slang>



layout(location = 0) in vec3 v_w_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_w_normal;
layout(location = 3) in vec3 v_w_tangent;

#if !SHADING_ENABLE_OPACITY_TEST
[earlydepthstencil]
#endif
[shader("pixel")]
vec4 main() : SV_Target0
{
	const vec2 uv = v_uv;
	var material = ReadBoundMaterial(uv, true);

	BoundScene scene = BoundScene();

	SurfaceGeometryInfo geom;
	geom.position = v_w_position;
	geom.vertex_shading_normal = Normalize(v_w_normal);

	geom.geometry_normal = geom.vertex_shading_normal;
	geom.geometry_normal = normalize(cross(ddy(geom.position), ddx(geom.position)));

// discard after using the screen space derivative 
#if SHADING_ENABLE_OPACITY_TEST
	if(scene.testOpacity(material.alpha))
	{
		discard;
	}
#endif

#if SHADING_FORCE_MAX_NORMAL_LEVEL >= SHADING_NORMAL_LEVEL_VERTEX
	geom.shading_normal = geom.vertex_shading_normal;
#else
	geom.shading_normal = geom.geometry_normal;
#endif

	geom.vertex_shading_tangent = SafeNormalize(v_w_tangent);
	geom.vertex_shading_tangent = SafeNormalize(geom.vertex_shading_tangent - dot(geom.vertex_shading_tangent, geom.vertex_shading_normal) * geom.vertex_shading_normal);
	
	ApplyMaterialNormalIFN(geom, material);

	let camera = MakeCamera(renderer_ubo.camera);
	const vec3 camera_position = camera.getPosition();
	const vec3 wo = normalize(camera_position - geom.position);

	var res = Shade(scene, geom, material);
	res.L += material.albedo * scene.getUBO().ambient;

	return vec4(res.L, 1);
}