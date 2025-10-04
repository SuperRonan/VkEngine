
#include <ShaderLib/ToneMap/ColorCorrections.slang>

layout(location = 0) in float4 color;
layout(location = 1) in float2 uv;

layout(location = 0) out float4 out_color;

layout(set = 0, binding = 0) uniform Sampler2D<float4> texture;

[vk::constant_id(0)]
const uint ColorCorrectionMode = 0;

struct PC
{
	[vk::offset(16)]
	float exposure;
	[vk::offset(24)]
	float gamma;
};

[vk::push_constant]
uniform PC _pc;

[shader("fragment")]
void main()
{
	out_color = color * texture.Sample(uv);
	
	out_color.xyz = OETF(out_color.xyz * _pc.exposure, TransferFunction(ColorCorrectionMode), _pc.gamma);
}