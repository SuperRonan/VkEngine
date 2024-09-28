#include "helper.hpp"

#include <format>

int GetInt(int i)
{
	return i + 1;
}

std::string GetString(int i)
{
	return std::format("{} -> {}", i, GetInt(i));
}