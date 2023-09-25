#pragma once

#include "ShaderBindingDescriptor.hpp"


std::string vkl::BindingIndex::asString() const
{
	std::stringstream ss;
	ss << *this;
	return ss.str();
}