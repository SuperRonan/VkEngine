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

// `wave_length` in [0, 1]
Vector3<Float> EvalXYZStimulus<Float : __BuiltinFloatingPointType>(Float wave_length)
{
	return Vector3<Float>(
		EvalXStimulus(wave_length),
		EvalYStimulus(wave_length),
		EvalZStimulus(wave_length)
	);
}

// Not normalized, the total energy level might be well outside the unit range!
// - `physical_wave_length` in nm
// - `temperature` in K
// See https://en.wikipedia.org/wiki/Planck%27s_law
float EvalBlackBodySpectralRadianceFromPhysical(float physical_wave_length, float temperature)
{
	physical_wave_length *= 1e-9; // map nm to m
	let lambda_5 = physical_wave_length * physical_wave_length * physical_wave_length * physical_wave_length * physical_wave_length;
	let h = 6.62607015e-34f;
	let c = 299792458.0f;
	let kB = 1.380649e-23f;
	let hc = h * c;
	let _2hc2 = 2.0f * hc * c;
	let e = exp(hc * rcp(physical_wave_length * kB * temperature));
	let res = _2hc2 * rcp(lambda_5 * (e - 1.0f));
	return res;
}

// Returns the integral of plank's law over the entire spectrum (or an approximation of it)
// - `temperature` in K
// See https://en.wikipedia.org/wiki/Stefan%E2%80%93Boltzmann_law
float EvalBlackBodyTotalSpectralRadiance(float temperature)
{
	// This is the total integral [0, +inf], not just restricted to the visible spectrum
	let total_spectral_energy = 4.067e-6 * pow(temperature, 5);
	return total_spectral_energy;
}

// Returns the integral of plank's law over the visible spectrum (or an approximation of it)
// - `temperature` in K
float EvalBlackBodyVisibleSpectralRadiance(float temperature)
{
	vec3 rgb = vec3(
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(0.25), temperature),
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(0.5), temperature),
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(0.75), temperature)
	);
	return Luminance(rgb) * sqrt(2.0f); // * sqrt(2) looks good, no mathematical justification of it
}

float EvalBlackBodySpectralRadiance(float temperature, bool total=false)
{
	return total ? EvalBlackBodyTotalSpectralRadiance(temperature) : EvalBlackBodyVisibleSpectralRadiance(temperature);
}

float EvalBlackBodySpectralRadiance(float temperature, int mode)
{
	return EvalBlackBodySpectralRadiance(temperature, mode == 2);
}