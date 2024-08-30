#pragma once

uint ModulateAxis(uint axis)
{
	if(axis >= 3)
	{
		axis -= 3;
	}
	return axis;
}

vec3 NormalFromAxis(uint axis)
{
	vec3 normal = 0..xxx;
	normal[axis] = 1;
	return normal;
}

mat3 BasisFromAxis(uint axis)
{
	return mat3(
		NormalFromAxis(ModulateAxis(axis + 1)),
		NormalFromAxis(ModulateAxis(axis + 2)),
		NormalFromAxis(axis)
	);
}

struct PC
{
	mat4 world_to_proj;
	uint flags;
	float line_count_f;
	float oo_line_count_minus_one;
	uint axis;
};

layout(push_constant) uniform PushConstant
{
	PC _pc;
};

uint GetLineCount()
{
	return _pc.flags & BIT_MASK(29);
}

uint GetAxisMask()
{
	return (_pc.flags >> 29) & BIT_MASK(3);
}