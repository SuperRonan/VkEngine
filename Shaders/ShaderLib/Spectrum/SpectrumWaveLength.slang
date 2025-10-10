#pragma once

#ifndef PHYSICAL_SPECTRUM_REDUCED
#define PHYSICAL_SPECTRUM_REDUCED 1
#endif

#if PHYSICAL_SPECTRUM_REDUCED
#define MIN_PHYSICAL_WAVE_LENGTH_NM 380
#define MAX_PHYSICAL_WAVE_LENGTH_NM 720
#else
#define MIN_PHYSICAL_WAVE_LENGTH_NM 380
#define MAX_PHYSICAL_WAVE_LENGTH_NM 750
#endif

#define PHYSICAL_WAVE_LENGTH_RANGE_NM (MAX_PHYSICAL_WAVE_LENGTH_NM - MIN_PHYSICAL_WAVE_LENGTH_NM)

#include <ShaderLib/Maths/AsymetricGaussian.slang>

// https://en.wikipedia.org/wiki/Visible_spectrum
// The visible range wavelengths of light [380nm, 750nm]: PhysicalWaveLength
// is mapped to [0, 1]: WaveLength (float)
	
// Returns in nm
Float GetPhysicalWaveLength<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	return Float(MIN_PHYSICAL_WAVE_LENGTH_NM) + Float(PHYSICAL_WAVE_LENGTH_RANGE_NM) * wave_length;
}

Float GetWaveLengthFromPhysical<Float : __BuiltinFloatingPointType>(Float physical_wave_length)
{
	return (physical_wave_length - Float(MIN_PHYSICAL_WAVE_LENGTH_NM)) * Float(rcp(PHYSICAL_WAVE_LENGTH_RANGE_NM));
}

AsymetricGaussian<Float> AsymetricGaussianFromPhysical<Float : __BuiltinFloatingPointType>(Float physical_mean, Float physical_tau1, Float physical_tau2)
{
	return AsymetricGaussian<Float>(GetWaveLengthFromPhysical(physical_mean), physical_tau1 * Float(PHYSICAL_WAVE_LENGTH_RANGE_NM), physical_tau2 * Float(PHYSICAL_WAVE_LENGTH_RANGE_NM));
}

// https://en.wikipedia.org/wiki/CIE_1931_color_space#Analytical_approximation:~:text=830%C2%A0nm.-,Analytical%20approximation,-%5Bedit%5D

// `wave_length in [0, 1]`
Float EvalXStimulus<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	Float res = 
		+ Float(1.056) * AsymetricGaussianFromPhysical(Float(599.8), Float(0.0264), Float(0.0323))(wave_length)
		+ Float(0.362) * AsymetricGaussianFromPhysical(Float(442.0), Float(0.0624), Float(0.0374))(wave_length)
		- Float(0.065) * AsymetricGaussianFromPhysical(Float(501.1), Float(0.0490), Float(0.0382))(wave_length)
	;
	return res;
}

// `wave_length in [0, 1]`
Float EvalYStimulus<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	Float res = 
		+ Float(0.821) * AsymetricGaussianFromPhysical(Float(568.8), Float(0.0213), Float(0.0247))(wave_length)
		+ Float(0.286) * AsymetricGaussianFromPhysical(Float(530.9), Float(0.0613), Float(0.0322))(wave_length)
	;
	return res;
}

// `wave_length in [0, 1]`
Float EvalZStimulus<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	Float res = 
		+ Float(1.217) * AsymetricGaussianFromPhysical(Float(437.0), Float(0.0845), Float(0.0278))(wave_length)
		+ Float(0.681) * AsymetricGaussianFromPhysical(Float(459.0), Float(0.0385), Float(0.0725))(wave_length)
	;
	return res;
}

// `wave_length in [0, 1]`
Vector3<Float> EvalXYZStimulus<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	return Vector3<Float>(
		EvalXStimulus(wave_length),
		EvalYStimulus(wave_length),
		EvalZStimulus(wave_length)
	);
}