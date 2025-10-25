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
	RNG_t rng = RNG_t(Hash(gid));	

	float x = (float(gid.x) + 0.5) / float(dims.x);
	Spectrum::Init(x);

	Spectrum spec = Spectrum(1);

	vec3 res = vec3(0);

	const float temperature = lerp(1000, 10000, x);

	if(gid.y < dims.y / 4)
	{
		// Show spectrum
		res = spec.toRGBf();
		if(gid.y > (3 * dims.y) / 16)
		{
			// Test spectrum -> rgb -> spectrum -> rgb
			vec3 src_rbg = res;
			res = vec3::Zero();
			const uint samples = 16;
			for(uint i = 0; i < samples; ++i)
			{
				SampledSpectrumf<4>::Init(rng.generate<float>());
				int method = 0;
				if(gid.y > (7 * dims.y) / 32)
				{
					method = 1;
				}
				res += SampledSpectrumf<4>::MakeFromRGB(src_rbg, RGBSourceType::Illuminant, method).toRGBf();
			}
			res /= float(samples);
		}
	}
	else if(gid.y < dims.y / 2)
	{
		// true Check that the avg is white
		// false Check color temperature
		bool mode = gid.y < (dims.y * 3) / 8;
		const uint samples = 4;
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
				spec = SampleSpectrumFromTemperature<Spec::Samples>(temperature, 2) * 8;
			}
			res += spec.toXYZf();
		}
		res = (GetXYZToRGBConversionMatrix<float>() * res) / float(samples);
		if((gid.y > (dims.y * 7) / 16))
		{
			int norm = 1; // visible
			if(gid.y < (dims.y * 15) / 32)
			{
				norm = 2; // total
			}
			res = SpectrumFromTemperature<SPECTRUM_MODE_RGB>(temperature, norm).toRGBf();
		}
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
	if((gid.x % 128) == 0 && gid.y == 0)
	{
		DebugPrinter printer = DebugPrinter(vec3(gid.x, 16, 0), DEBUG_PIXEL_SPACE_BIT);
		printer.print(int(GetPhysicalWaveLength(BaseWaveLength)), "nm");
		printer.print(int(temperature), "K");
	}
}