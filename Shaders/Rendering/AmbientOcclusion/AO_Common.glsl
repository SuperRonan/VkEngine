#pragma once

#define USE_NORMAL_TEXTURE_BIT 0x1

#define AO_METHOD_SSAO 0
#define AO_METHOD_RTAO 1
#define AO_METHOD_RQAO 2

struct UBO_AO
{
	float radius;
};