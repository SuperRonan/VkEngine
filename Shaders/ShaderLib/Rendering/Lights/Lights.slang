#pragma once

#include "PointLight.slang"
#include "DirectionalLight.slang"
#include "SpotLight.slang"
#include "BeamLight.slang"

ILight<Mode> MakeLight<int Mode>(const in StorageLight l)
{
	const LightType type = l.getType();
	if(type == LightType::Point)
	{
		return PointLight<Mode>(l);
	}
	else if(type == LightType::Directional)
	{
		return DirectionalLight<Mode>(l);
	}
	else if(type == LightType::Spot)
	{
		return SpotLight<Mode>(l);
	}
	else// if(type == LightType::Beam)
	{
		return BeamLight<Mode>(l);
	}
}