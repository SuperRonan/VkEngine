#pragma once

#include <ShaderLib:/common.glsl>
#include <ShaderLib:/Rendering/Lights/Light.glsl>


#ifndef BIND_SCENE
#define BIND_SCENE 1
#endif


#if BIND_SCENE

#define SCENE_BINDING SCENE_DESCRIPTOR_BINDING

#ifndef LIGHTS_ACCESS
#define LIGHTS_ACCESS
#endif

layout(SCENE_BINDING + 0) uniform SceneUBOBinding
{
	vec3 ambient;
	uint num_lights;
} scene_ubo;

layout(SCENE_BINDING + 1) restrict LIGHTS_ACCESS buffer LightsBufferBinding
{
	Light lights[];
} lights_buffer;

#endif