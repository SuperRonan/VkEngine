#pragma once

#include <ShaderLib/common.slang>

__generic <Float : __BuiltinFloatingPointType>
static Matrix3<Float> GetRGBToXYZConversionMatrix()
{
	return Matrix3<Float>(
		Float(0.49000), Float(0.31000), Float(0.20000),
		Float(0.17697), Float(0.81240), Float(0.01063),
		Float(0.00000), Float(0.01000), Float(0.99000)
	);
}

__generic <Float : __BuiltinFloatingPointType>
static Matrix3<Float> GetXYZToRGBConversionMatrix()
{
	return Matrix3<Float>(
		Float(2.36461385), Float(-0.89654057), Float(-0.46807328),
		Float(-0.51516621), Float(1.4264081), Float(0.0887581),
		Float(0.0052037), Float(-0.01440816), Float(1.00920446)
	);
}