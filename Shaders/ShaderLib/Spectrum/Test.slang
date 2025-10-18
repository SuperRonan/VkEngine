#include "Spectrum.slang"

#include <ShaderLib/Color.slang>

#define I_WANT_TO_DEBUG 1
#include <ShaderLib/Debug/DebugBuffers.slang>

#ifndef TARGET_FORMAT
#define TARGET_FORMAT rgba16f
#endif

layout(SHADER_DESCRIPTOR_BINDING + 0,TARGET_FORMAT)
RWTexture2D<float4> target;

[shader("compute")]
[NumThreads(1, 1, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
	typealias Spectrum = SingleSpectrumf;
	uint2 dims;
	target.GetDimensions(dims.x, dims.y);

	float x = (float(gid.x) + 0.5) / float(dims.x);
	Spectrum::Init(x);

	Spectrum spec = Spectrum(1);

	vec3 res = vec3(0);

	const float temperature = lerp(1000, 10000, x);

	if(gid.y < dims.y / 4)
	{
		// Show spectrum
		res = spec.toRGBf();
	}
	else if(gid.y < dims.y / 2)
	{
		// true Check that the avg is white
		// false Check color temperature
		bool mode = gid.y < (dims.y * 3) / 8;
		const uint samples = 4;
		RNG_t rng = RNG_t(Hash(gid));
		for(uint i = 0; i < samples; ++i)
		{
			typealias Spec = SampledSpectrumf<4>;
			Spec::Init(rng.generate<float>());
			Spec spec = {};
			if(mode)
			{
				spec = Spec(2);
			}
			else
			{
				spec = SampleSpectrumFromTemperature<Spec::Samples>(temperature, true) * 8;
			}
			res += spec.toXYZf();
		}
		res = (GetXYZToRGBConversionMatrix<float>() * res) / float(samples);
	}
	else
	{
		// Show individual XYZ responses
		uint index = (gid.y / (dims.y / 6)) % 3;
		uint h = gid.y % (dims.y / 6);
		float y = EvalXYZStimulus(x)[index] * 0.5;
		if((y * dims.y / 6) >= (dims.y / 6 - h))
		{
			res = spec.toRGBf();
		}
	}

	target.Store(gid.xy, vec4(res, 0));

	// Show nm scale
	if((gid.x % 128) == 0)
	{
		DebugPrinter printer = DebugPrinter(vec3(gid.x, 16, 0), DEBUG_PIXEL_SPACE_BIT);
		printer.print(int(GetPhysicalWaveLength(BaseWaveLength)), "nm");
		printer.print(int(temperature), "K");
	}
}