#pragma once

namespace ez80
{
	template<typename Elem = char>
	constexpr bool IsIdentifierStart(Elem elem) noexcept;

	template<typename Elem = char>
	constexpr bool IsParameterSeparator(Elem elem) noexcept;
}

#include "AssemblerStringUtil.inl"
