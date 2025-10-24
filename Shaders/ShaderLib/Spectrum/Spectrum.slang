#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/ColorSpace.slang>
#include "SpectrumWaveLength.slang"

#include <ShaderLib/Maths/Lerp.slang>

// Could be moved to a common helper header
#define FOREACH_ARITHMETIC_OP(X) \
	X(add, +) \
	X(sub, -) \
	X(mul, *) \
	X(div, /) \
	X(mod, %)

#define _SPECTRUM_SAMPLES_BIT_COUNT 16
#define _SPECTRUM_FLAG_RGB (0x0 << _SPECTRUM_SAMPLES_BIT_COUNT)
#define _SPECTRUM_FLAG_XYZ (0x1 << _SPECTRUM_SAMPLES_BIT_COUNT)
#define _SPECTRUM_FLAG_SAMPLED (0x2 << _SPECTRUM_SAMPLES_BIT_COUNT)
#define _SPECTRUM_FLAG_MASK (0x3 << _SPECTRUM_SAMPLES_BIT_COUNT)

#define SPECTRUM_MODE_RGB (_SPECTRUM_FLAG_RGB | 3)
#define SPECTRUM_MODE_XYZ (_SPECTRUM_FLAG_XYZ | 3)
#define SPECTRUM_MODE_SAMPLED(Samples) (_SPECTRUM_FLAG_SAMPLED | Samples)

// TODO
// Common for all SampledSpectrum in the thread
// Idealy, it would be a static variable of SampledSpectrumHelper<N>
// But for now, slang does not support generic struct's static variables
static float BaseWaveLength;

__generic <int Samples>
struct SampledSpectrumHelper
{
	static float SingleChannelBandwith()
	{
		return rcp(float(Samples));
	}

	// xi in [0, 1]
	[mutating]
	static void Init(float xi)
	{
		BaseWaveLength = lerp(0, SingleChannelBandwith(), xi);
	}

	static float GetChannelWaveLength(uint i)
	{
		return BaseWaveLength + float(i) * SingleChannelBandwith();
	}
};

enum RGBSourceType
{
	Reflectance,
	Illuminant,
};
typealias XYZSourceType = RGBSourceType;

#define AUTODIFF

__generic <Scalar : __BuiltinFloatingPointType, int Mode>
struct Spectrum : IRWArray<Scalar>, IFloat
{
	static const int Samples = Mode & BIT_MASK(_SPECTRUM_SAMPLES_BIT_COUNT);
	static const bool IsRGB = (Mode & _SPECTRUM_FLAG_MASK) == _SPECTRUM_FLAG_RGB;
	static const bool IsXYZ = (Mode & _SPECTRUM_FLAG_MASK) == _SPECTRUM_FLAG_XYZ;
	static const bool IsSampled = (Mode & _SPECTRUM_FLAG_MASK) == _SPECTRUM_FLAG_SAMPLED;

	typealias VecT = vector<Scalar, Samples>;
	VecT _data;

	typealias Helper = SampledSpectrumHelper<Samples>;

	__init()
	{
		_data = {};
	}

	__init(int v)
	{
		_data = VecT(Scalar(v));
	}

	__init(Scalar v)
	{
		_data = VecT(v);
	}

	__init(const in This other)
	{
		_data = other._data;
	}

	__generic<OtherScalar : __BuiltinFloatingPointType>
	__init(const in Spectrum<OtherScalar, Mode> other)
	{
		_data = VecT(other._data);
	}

	__generic<OtherScalar : __BuiltinFloatingPointType>
	__init(const in vector<OtherScalar, Samples> vec)
	{
		_data = VecT(vec);
	}

	static This Uniform(Scalar s)
	{
		return This(s);
	}

	static This Zero()
	{
		return This(0);
	}

	static This One()
	{
		return This(1);
	}

	static float GetChannelWaveLength(int i)
	{
		if(IsSampled)
		{
			return Helper::GetChannelWaveLength(uint(i));
		}
		else
		{
			// GoodEnough RGB approx
			return 1.0f - float(i + 1) / float(Samples + 1);
		}
	}

	[mutating]
	static void Init(float xi)
	{
		if(IsSampled)
		{
			Helper::Init(xi);
		}
	}

	__generic<Float : __BuiltinFloatingPointType>
	Vector3<Float> toXYZ()
	{
		if(IsRGB)
		{
			return Vector3<Float>(GetRGBToXYZConversionMatrix<Scalar>() * (_data as Vector3<Scalar>).value);
		}
		else if(IsXYZ)
		{	
			return Vector3<Float>((_data as Vector3<Scalar>).value);
		}
		else
		{
			Vector3<Float> res = Vector3<Float>::Zero();
			for(int i = 0; i < Samples; ++i)
			{
				res += ConvertBuiltInTo<Float>(_data[i]) * Vector3<Float>(EvalXYZStimulus(GetChannelWaveLength(i)));
			}
			return res / Float(Samples);
		}
	}

	vec3 toXYZf()
	{
		return toXYZ<float>();
	}

	__generic<Float : __BuiltinFloatingPointType>
	Vector3<Float> toRGB()
	{
		if(IsRGB)
		{
			return Vector3<Float>((_data as Vector3<Scalar>).value);
		}
		else if(IsXYZ)
		{	
			return Vector3<Float>(GetXYZToRGBConversionMatrix<Scalar>() * (_data as Vector3<Scalar>).value);
		}
		else
		{
			return Vector3<Float>(GetXYZToRGBConversionMatrix<Float>() * toXYZ<Float>());
		}
	}

	vec3 toRGBf()
	{
		return toRGB<float>();
	}

	__generic<Float : __BuiltinFloatingPointType>
	static This MakeFromRGB(const in Vector3<Float> rgb, RGBSourceType from_type, int method)
	{
		if(IsRGB)
		{
			return (Spectrum<Scalar, SPECTRUM_MODE_RGB>(rgb) as This).value;
		}
		else if(IsXYZ)
		{
			return MakeFromXYZ(GetRGBToXYZConversionMatrix<Float>() * rgb, from_type);
		}
		else
		{
			This res = This::Zero();
			const float scale = from_type == RGBSourceType::Reflectance ? 1 : 3;
			// TODO this correctly
			for(int i = 0; i < Samples; ++i)
			{
				// This not as bad an approximation as it looks
				uint channel = 0;
				channel = 2 - uint(GetChannelWaveLength(i) * 3);
				res[i] += ConvertBuiltInTo<Scalar>(rgb[channel]).scale(scale);
			}
			return res;
		}
	}

	__generic<Float : __BuiltinFloatingPointType>
	static This MakeFromRGB(const in Vector3<Float> rgb, RGBSourceType from_type)
	{
		return MakeFromRGB(rgb, from_type, 0);
	}

	__generic<Float : __BuiltinFloatingPointType>
	static This MakeFromXYZ(const in Vector3<Float> xyz, XYZSourceType from_type)
	{
		if(IsXYZ)
		{
			return (Spectrum<Scalar, SPECTRUM_MODE_XYZ>(xyz) as This).value;
		}
		else
		{
			return MakeFromRGB(GetXYZToRGBConversionMatrix<Float>() * xyz, from_type);
		}
	}

	// 	float energy()
	// {
	// 	if(IsRGB)
	// 	{
	// 		return Luminance(vec3((_data as Vector3<Scalar>).value));
	// 	}
	// 	else
	// 	{
	// 		return fastEnergy();
	// 	}
	// }

	float fastEnergy()
	{
		if(IsRGB)
		{
			return FastLuminance(toRGBf());
		}
		else if(IsXYZ)
		{
			return _data[1].toFloat();
		}
		else // if (IsSampled)
		{
			float res = 0;
			if(Samples == 1)
			{
				res = _data[0].toFloat();
			}
			else
			{
				for(int i = 0; i < Samples; ++i)
				{
					res += EvalYStimulus(GetChannelWaveLength(i)) * _data[i].toFloat();
				}
			}
			return res;
		}
	}

	bool isNonZero()
	{
		return NonZero(_data);
	}


	int getCount()
	{
		return Samples;
	}

	__subscript(int i) -> Scalar
	{
		get {return _data[i]; }
		set {_data[i] = newValue; }
	}

#pragma warning (push)
#pragma warning (disable : 15503) // token pasting warning
#define DECLARE_SPECTRUM_ARITHMETIC_OP(MEMBER, OP) \
	AUTODIFF \
	This MEMBER(const in This rhs) \
	{ \
		return This(this._data.MEMBER(rhs._data)); \
	} \
	AUTODIFF \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	This MEMBER(const in Spectrum<OtherScalar, Mode> rhs) \
	{ \
		return This(this._data.MEMBER(VecT(rhs._data))); \
	} \
	AUTODIFF \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	This MEMBER(const in OtherScalar s) \
	{ \
		return This(this._data.MEMBER(ConvertBuiltInTo<Scalar>(s))); \
	}

	FOREACH_ARITHMETIC_OP(DECLARE_SPECTRUM_ARITHMETIC_OP)

#undef DECLARE_SPECTRUM_ARITHMETIC_OP
#pragma warning (pop)

	AUTODIFF
	This neg()
	{
		return This(this._data.neg());
	}

	AUTODIFF
	This operator-()
	{
		return neg();
	}

	AUTODIFF
	__generic <OtherScalar : __BuiltinFloatingPointType>
	This scale(OtherScalar s)
	{
		return this.mul(s);
	}

	float toFloat()
	{
		return _data[0].toFloat();
	}

	bool equals(This other)
	{
		// TODO
		// _data.equals(other._data); currently generates an internal compiler error in slangc
		return false;//_data.equals(other._data);
	}

	bool lessThan(This other)
	{
		return _data.lessThan(other._data);
	}

	bool lessThanOrEquals(This other)
	{
		return _data.lessThanOrEquals(other._data);
	}
};

#pragma warning (push)
#pragma warning (disable : 15503) // token pasting warning
#define DECLARE_SPECTRUM_ARITHMETIC_OP(MEMBER, OP) \
	AUTODIFF \
	__generic <Scalar : __BuiltinFloatingPointType, int Mode, OtherScalar : __BuiltinFloatingPointType> \
	Spectrum<Scalar, Mode> operator##OP(const in Spectrum<Scalar, Mode> lhs, const in Spectrum<OtherScalar, Mode> rhs) \
	{ \
		return lhs.MEMBER(rhs); \
	} \
	AUTODIFF \
	__generic <Scalar : __BuiltinFloatingPointType, int Mode, OtherScalar : __BuiltinFloatingPointType> \
	Spectrum<Scalar, Mode> operator##OP(const in Spectrum<Scalar, Mode> lhs, const in OtherScalar rhs) \
	{ \
		return lhs.MEMBER(rhs); \
	} \
	__generic <Scalar : __BuiltinFloatingPointType, int Mode, OtherScalar : __BuiltinFloatingPointType> \
	Spectrum<Scalar, Mode> operator##OP(const in OtherScalar lhs, const in Spectrum<Scalar, Mode> rhs) \
	{ \
		return rhs.MEMBER(lhs); \
	} \
	AUTODIFF \
	__generic <Scalar : __BuiltinFloatingPointType, int Mode, OtherScalar : __BuiltinFloatingPointType> \
	void operator##OP##=(inout Spectrum<Scalar, Mode> lhs, const in Spectrum<OtherScalar, Mode> rhs) \
	{ \
		lhs._data OP##= Spectrum<Scalar, Mode>::VecT(rhs._data); \
	} \
	AUTODIFF \
	__generic <Scalar : __BuiltinFloatingPointType, int Mode, OtherScalar : __BuiltinFloatingPointType> \
	void operator##OP##=(inout Spectrum<Scalar, Mode> lhs, const in OtherScalar rhs) \
	{ \
		lhs._data OP##= ConvertBuiltInTo<Scalar>(rhs); \
	}

FOREACH_ARITHMETIC_OP(DECLARE_SPECTRUM_ARITHMETIC_OP)
#undef DECLARE_SPECTRUM_ARITHMETIC_OP
#pragma warning (pop)

__generic <int Mode>
typealias Spectrumf = Spectrum<float, Mode>;
__generic <int Mode>
typealias Spectrumh = Spectrum<half, Mode>;

__generic <Scalar : __BuiltinFloatingPointType, int Samples>
typealias SampledSpectrum = Spectrum<Scalar, SPECTRUM_MODE_SAMPLED(Samples)>;

__generic <int Samples>
typealias SampledSpectrumf = SampledSpectrum<float, Samples>;
__generic <int Samples>
typealias SampledSpectrumh = SampledSpectrum<half, Samples>;

__generic <Scalar : __BuiltinFloatingPointType>
typealias SingleSpectrum = SampledSpectrum<Scalar, SPECTRUM_MODE_SAMPLED(1)>;
typealias SingleSpectrumf = SingleSpectrum<float>;
typealias SingleSpectrumh = SingleSpectrum<half>;

__generic <Scalar : __BuiltinFloatingPointType>
typealias RGBSpectrum = Spectrum<Scalar, SPECTRUM_MODE_RGB>;
typealias RGBSpectrumf = RGBSpectrum<float>;
typealias RGBSpectrumh = RGBSpectrum<half>;

__generic <Scalar : __BuiltinFloatingPointType>
typealias XYZSpectrum = Spectrum<Scalar, SPECTRUM_MODE_XYZ>;
typealias XYZSpectrumf = XYZSpectrum<float>;
typealias XYZSpectrumh = XYZSpectrum<half>;

__generic <Scalar : __BuiltinFloatingPointType>
extension RGBSpectrum<Scalar>
{
#define RGB_SPECTRUM_DECLARE_CHANNEL_PROP(NAME, LOCATION) \
	property Scalar NAME \
	{ \
		get { \
			return this._data.LOCATION; \
		} \
		set { \
			this._data.LOCATION = newValue; \
		} \
	}

	RGB_SPECTRUM_DECLARE_CHANNEL_PROP(r, x)
	RGB_SPECTRUM_DECLARE_CHANNEL_PROP(g, y)
	RGB_SPECTRUM_DECLARE_CHANNEL_PROP(b, z)

#undef RGB_SPECTRUM_DECLARE_CHANNEL_PROP
}

__generic <Scalar : __BuiltinFloatingPointType>
extension XYZSpectrum<Scalar>
{
#define XYZ_SPECTRUM_DECLARE_CHANNEL_PROP(LOCATION) \
	property Scalar LOCATION \
	{ \
		get { \
			return this._data.LOCATION; \
		} \
		set { \
			this._data.LOCATION = newValue; \
		} \
	}

	XYZ_SPECTRUM_DECLARE_CHANNEL_PROP(x)
	XYZ_SPECTRUM_DECLARE_CHANNEL_PROP(y)
	XYZ_SPECTRUM_DECLARE_CHANNEL_PROP(z)

#undef XYZ_SPECTRUM_DECLARE_CHANNEL_PROP
}

namespace impl
{
	Spectrumf<Mode> EvalSpectrumAtTemperature<int Mode>(float temperature, bool normalize=false)
	{
		Spectrumf<Mode> res;
		let norm = EvalBlackBodyTotalSpectralRadiance(temperature);
		for(uint i = 0; i < res::Samples; ++i)
		{
			res[i] = EvalBlackBodySpectralRadianceFromPhysical(GetPhysicalWaveLength(res.GetChannelWaveLength(i)), temperature);
			if(normalize)
			{
				res[i] /= norm;
			}
		}
		return res;
	}
}

SampledSpectrumf<N> SampleSpectrumFromTemperature<int N>(float temperature, bool normalize=false)
{
	return impl::EvalSpectrumAtTemperature<SPECTRUM_MODE_SAMPLED(N)>(temperature, normalize);
}

Spectrumf<Mode> SpectrumFromTemperature<int Mode>(float temperature, bool normalize=false)
{
	typealias SpectruM = Spectrumf<Mode>;
	SpectruM res;
	if(SpectruM::IsSampled)
	{
		res = (SampleSpectrumFromTemperature<Mode>(temperature, normalize) as SpectruM).value;
	}
	else if(SpectruM::IsRGB)
	{
		res = (impl::EvalSpectrumAtTemperature<Mode>(temperature, normalize) as SpectruM).value * 3.0f;
	}
	else if(SpectruM::IsXYZ)
	{
		res = SpectruM::MakeFromXYZ((impl::EvalSpectrumAtTemperature<SPECTRUM_MODE_RGB>(temperature, normalize) * 3.0f).toXYZf(), XYZSourceType::Illuminant);
	}
	
	return res;
}