#pragma once

#include <ShaderLib/common.slang>
#include <ShaderLib/ColorSpace.slang>
#include "SpectrumWaveLength.slang"

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
			return 1.0f - float(i + 1) / float(Samples);
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

	__generic<Float : __BuiltinFloatingPointType, RGBSourceType _FromType>
	static This MakeFromRGB(const in Vector3<Float> rgb)
	{
		if(IsRGB)
		{
			return (Spectrum<Scalar, SPECTRUM_MODE_RGB>(rgb) as This).value;
		}
		else if(IsXYZ)
		{
			return (Spectrum<Scalar, SPECTRUM_MODE_XYZ>(GetRGBToXYZConversionMatrix<Float>() * rgb) as This).value;
		}
		else
		{
			This res = This::Zero();
			for(int i = 0; i < Samples; ++i)
			{
				uint channel = uint(GetChannelWaveLength(i) * 3);
				res[channel] += ConvertBuiltInTo<Scalar>(rgb[channel]);
			}
			return res;
		}
	}

	// __generic<Float : __BuiltinFloatingPointType>
	// static This MakeFromXYZ(const in Vector3<Float> xyz)
	// {

	// }

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
	This operator##OP(const in Spectrum<OtherScalar, Mode> rhs) \
	{ \
		return This(this._data.MEMBER(VecT(rhs._data))); \
	} \
	AUTODIFF \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	This operator##OP(const in OtherScalar s) \
	{ \
		return This(this._data.MEMBER(ConvertBuiltInTo<Scalar>(s))); \
	} \
	AUTODIFF \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	This MEMBER(const in OtherScalar s) \
	{ \
		return This(this._data.MEMBER(ConvertBuiltInTo<Scalar>(s))); \
	} \
	AUTODIFF \
	[mutating] \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	void operator##OP##=(const in Spectrum<OtherScalar, Mode> rhs) \
	{ \
		this._data OP##= VecT(rhs._data); \
	} \
	AUTODIFF \
	[mutating] \
	__generic <OtherScalar : __BuiltinFloatingPointType> \
	void operator##OP##=(const in OtherScalar s) \
	{ \
		this._data OP##= ConvertBuiltInTo<Scalar>(s); \
	}

	DECLARE_SPECTRUM_ARITHMETIC_OP(add, +)
	DECLARE_SPECTRUM_ARITHMETIC_OP(sub, -)
	DECLARE_SPECTRUM_ARITHMETIC_OP(mul, *)
	DECLARE_SPECTRUM_ARITHMETIC_OP(div, /)
	DECLARE_SPECTRUM_ARITHMETIC_OP(mod, %)

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
}

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