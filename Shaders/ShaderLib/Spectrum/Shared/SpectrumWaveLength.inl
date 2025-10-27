#pragma once

#include <ShaderLib/interop_slang_cpp>

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

// https://en.wikipedia.org/wiki/Visible_spectrum
// The visible range wavelengths of light [380nm, 750nm]: PhysicalWaveLength (in nm)
// is mapped to [0, 1]: WaveLength (float)
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float GetPhysicalWaveLength(Float wave_length, Float min, Float max)
{
	return CPP_ONLY(std::)lerp(min, max, wave_length);
}

// Returns in nm
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float GetPhysicalWaveLength(Float wave_length)
{
	return GetPhysicalWaveLength<Float>(wave_length, Float(MIN_PHYSICAL_WAVE_LENGTH_NM), Float(MAX_PHYSICAL_WAVE_LENGTH_NM));
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float GetWaveLengthFromPhysical(Float physical_wave_length, Float min, Float max)
{
	return (physical_wave_length - min) / (max - min);
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float GetWaveLengthFromPhysical(Float physical_wave_length)
{
	return GetWaveLengthFromPhysical<Float>(physical_wave_length, Float(MIN_PHYSICAL_WAVE_LENGTH_NM), Float(MAX_PHYSICAL_WAVE_LENGTH_NM));
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
AsymetricGaussian<Float> AsymetricGaussianFromPhysical(Float physical_mean, Float physical_tau1, Float physical_tau2)
{
	let_auto mu = GetWaveLengthFromPhysical(physical_mean);
	let_auto tau1 = physical_tau1 * Float(PHYSICAL_WAVE_LENGTH_RANGE_NM);
	let_auto tau2 = physical_tau2 * Float(PHYSICAL_WAVE_LENGTH_RANGE_NM);
	return AsymetricGaussian<Float>(mu, tau1, tau2);
}

// https://en.wikipedia.org/wiki/CIE_1931_color_space#Analytical_approximation:~:text=830%C2%A0nm.-,Analytical%20approximation,-%5Bedit%5D

// `wave_length in [0, 1]`
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float EvalXStimulus(Float wave_length)
{
	Float res = 
		+ Float(1.056) * AsymetricGaussianFromPhysical(Float(599.8), Float(0.0264), Float(0.0323))(wave_length)
		+ Float(0.362) * AsymetricGaussianFromPhysical(Float(442.0), Float(0.0624), Float(0.0374))(wave_length)
		- Float(0.065) * AsymetricGaussianFromPhysical(Float(501.1), Float(0.0490), Float(0.0382))(wave_length)
	;
	return res;
}

// `wave_length in [0, 1]`
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float EvalYStimulus(Float wave_length)
{
	Float res = 
		+ Float(0.821) * AsymetricGaussianFromPhysical(Float(568.8), Float(0.0213), Float(0.0247))(wave_length)
		+ Float(0.286) * AsymetricGaussianFromPhysical(Float(530.9), Float(0.0613), Float(0.0322))(wave_length)
	;
	return res;
}

// `wave_length in [0, 1]`
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Float EvalZStimulus(Float wave_length)
{
	Float res = 
		+ Float(1.217) * AsymetricGaussianFromPhysical(Float(437.0), Float(0.0845), Float(0.0278))(wave_length)
		+ Float(0.681) * AsymetricGaussianFromPhysical(Float(459.0), Float(0.0385), Float(0.0725))(wave_length)
	;
	return res;
}

// `wave_length` in [0, 1]
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
Vector3<Float> EvalXYZStimulus(Float wave_length)
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
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
constexpr Float EvalBlackBodySpectralRadianceFromPhysical(Float physical_wave_length, Float temperature)
{
	physical_wave_length *= Float(1e-9); // map nm to m
	let_auto lambda_5 = physical_wave_length * physical_wave_length * physical_wave_length * physical_wave_length * physical_wave_length;
	let_auto h = Float(6.62607015e-34);
	let_auto c = Float(299792458);
	let_auto kB = Float(1.380649e-23);
	let_auto hc = h * c;
	let_auto _2hc2 = Float(2) * hc * c;
	let_auto e = CPP_ONLY(std::)exp(hc / (physical_wave_length * kB * temperature));
	let_auto res = _2hc2 / (lambda_5 * (e - Float(1)));
	return res;
}

// Returns the integral of plank's law over the entire spectrum (or an approximation of it)
// - `temperature` in K
// See https://en.wikipedia.org/wiki/Stefan%E2%80%93Boltzmann_law
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
constexpr Float EvalBlackBodyTotalSpectralRadiance(Float temperature)
{
	// This is the total integral [0, +inf], not just restricted to the visible spectrum
	let_auto total_spectral_energy = Float(4.067e-6) * (temperature * temperature * temperature * temperature * temperature);
	return total_spectral_energy;
}

// Returns the integral of plank's law over the visible spectrum (or an approximation of it)
// - `temperature` in K
__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
constexpr Float EvalBlackBodyVisibleSpectralRadiance(Float temperature)
{
	Vector3<Float> rgb = Vector3<Float>(
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(Float(0.75)), temperature),
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(Float(0.50)), temperature),
		EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(Float(0.25)), temperature)
	);
	return Luminance(rgb) * CPP_ONLY(std::)sqrt(Float(2)); // * sqrt(2) looks good, no mathematical justification of it
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
constexpr Float EvalBlackBodySpectralRadiance(Float temperature, bool total=false)
{
	return total ? EvalBlackBodyTotalSpectralRadiance(temperature) : EvalBlackBodyVisibleSpectralRadiance(temperature);
}

__generic <CONCEPT_TYPE(FLOATING_POINT_CONCEPT, Float)>
constexpr Float EvalBlackBodySpectralRadianceNorm(Float temperature, uint mode)
{
	if (mode != 0)
	{
		return EvalBlackBodySpectralRadiance(temperature, mode == 2);
	}
	else
	{
		return Float(1);
	}
}
