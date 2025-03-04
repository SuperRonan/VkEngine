#pragma once

#include <ShaderLib/common.slang>

#include <ShaderLib/Maths/AABB.slang>

template <typename T : __BuiltinArithmeticType, int N>
interface IGeometry
{

	AlignedAxisBoundingBox<T, N> getAABB();


}

