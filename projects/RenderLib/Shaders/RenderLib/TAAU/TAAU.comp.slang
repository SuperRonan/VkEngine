
#include "common.slang"
#include <ShaderLib/Texturing.slang>

layout(SHADER_DESCRIPTOR_BINDING + 1, IMAGE_FORMAT) uniform restrict readonly RWTexture2D<float4> Input;

layout(SHADER_DESCRIPTOR_BINDING + 2, IMAGE_FORMAT) uniform restrict RWTexture2D<float4> Output;

struct PushConstant
{
	float alpha;
	uint flags;
};

[vk::push_constant]
uniform PushConstant _pc;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(const uvec3 GlobalInvocationID : SV_DispatchThreadID)
{
	const uvec2 gid = uvec2(GlobalInvocationID.xy);
	const uvec2 pixel = gid;
	const uvec2 dims = TextureSize(Output);
	if(all(pixel <= dims))
	{
		const vec4 new_data = Input.Load(pixel);

		const float alpha = _pc.alpha;

		vec4 result = 0..xxxx;
		if((_pc.flags & TAAU_FLAG_RESET_BIT) != 0)
		{
		 	result = new_data;
			result.w = 1;
		}
		else
		{
			const vec4 old_data = Output.Load(pixel);
			// float precision issue as lim alpha -> 1
			// The image darkens
			// On host, the alpha is limited to a numer of samples before the darkening is too visible
			result.xyz = lerp(new_data.xyz, old_data.xyz, alpha);
		}

		Output.Store(pixel, result);
	}
}