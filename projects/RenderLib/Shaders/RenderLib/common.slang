#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Rendering/Camera/camera.slang>

#ifndef RENDERER_BINDING
#define RENDERER_BINDING MODULE_DESCRIPTOR_BINDING + 0
#endif

#define RENDERER_BINDING_COUNT 1

#ifndef BIND_RENDERER_SET
#define BIND_RENDERER_SET 0
#endif

struct RendererUBO
{
	float time;
	float delta_time;
	uint frame_idx;
	uint _pixel_jitter_pack_u16x2_snorm; // in [-1, 1]

	vec2 oo_render_resolution;
	uint _render_resolution_pack_u16x2; // Accessing Vector2<u16> through UBO requires an extension, unpacking does not
	uint _output_resolution_pack_u16x2; // Accessing Vector2<u16> through UBO requires an extension, unpacking does not

	StorageCamera camera;

	property render_resolution : Vector2<u16>
	{
		get { return bit_cast<Vector2<u16>>(_render_resolution_pack_u16x2); }
	}

	property output_resolution : Vector2<u16>
	{
		get { return bit_cast<Vector2<u16>>(_output_resolution_pack_u16x2); }
	}

	property pixel_jitter_m11 : vec2
	{
		get { return unpackSnorm2x16ToFloat(_pixel_jitter_pack_u16x2_snorm); }
	}

	property pixel_jitter_m11_f16 : half2
	{
		get { return unpackSnorm2x16ToHalf(_pixel_jitter_pack_u16x2_snorm); }
	}

	property pixel_jitter_01 : vec2
	{
		get { return ClipSpaceToUV(pixel_jitter_m11); }
	}

	vec2 getCameraMatrixJitter()
	{
		return pixel_jitter_m11 * oo_render_resolution;
	}

	vec2 getUVJacobianScaling()
	{
		return rcp(vec2(output_resolution) * oo_render_resolution);
	}
};

#if BIND_RENDERER_SET


layout(RENDERER_BINDING + 0) uniform ConstantBuffer<RendererUBO> renderer_ubo;

layout(RENDERER_BINDING + 1) uniform SamplerComparisonState LightDepthSampler;

#endif