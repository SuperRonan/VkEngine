
#include "common.slang"
#include <ShaderLib/Texturing.slang>

layout(SHADER_DESCRIPTOR_BINDING + 1, IMAGE_FORMAT) uniform restrict readonly RWTexture2D<float4> Input;

layout(SHADER_DESCRIPTOR_BINDING + 2, IMAGE_FORMAT) uniform restrict RWTexture2D<float4> Output;

struct PushConstant
{
	float new_sample_weight;
	uint flags;
	uint new_pixel_location_packed_u16x2;

	property new_pixel_location : Vector2<u16>
	{
		get {return bit_cast<Vector2<u16>>(new_pixel_location_packed_u16x2);}
	}
};

[vk::push_constant]
uniform PushConstant _pc;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(const uvec3 GlobalInvocationID : SV_DispatchThreadID)
{
	const uvec2 gid = uvec2(GlobalInvocationID.xy);
	const uvec2 pixel = gid;
	const uvec2 out_dims = TextureSize(Output);
	const uvec2 in_dims = TextureSize(Input);
	if(any(pixel > in_dims))
	{
		return;
	}
	// Only consider integral scaling
	const uvec2 scaling = out_dims / in_dims;
	let new_pixel_location = _pc.new_pixel_location;
	if(any(new_pixel_location > out_dims))
	{
		return;
	}
	const uvec2 target_pixel = pixel * scaling + new_pixel_location;
	const vec4 new_data = Input.Load(pixel);

	const float new_sample_weight = _pc.new_sample_weight;

	vec4 result = 0..xxxx;
	if((_pc.flags & TAAU_FLAG_RESET_BIT) != 0)
	{
		result = new_data;
		result.w = 1;
	}
	else
	{
		const vec4 old_data = Output.Load(target_pixel);
		// float precision issue as lim alpha -> 0
		// The image darkens
		// On host, the alpha is limited to a numer of samples before the darkening is too visible
		result.xyz = lerp(old_data.xyz, new_data.xyz, new_sample_weight);
	}

	Output.Store(target_pixel, result);
}