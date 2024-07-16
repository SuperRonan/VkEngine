#pragma once

namespace std
{
    template <class T, bool Use>
	struct CompileTimeOptional
	{
		static const constexpr bool hasValue = Use;
	};

	template <class T>
	struct CompileTimeOptional<T, true>
	{
		static const constexpr bool hasValue = true;
		T _;
	};
}