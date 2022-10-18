#include "AssemblerStringUtil.h"
#include "StringUtil.h"

namespace ez80
{
	template<typename Elem>
	constexpr bool IsIdentifierStart(Elem elem) noexcept
	{
		return util::string::IsAlpha(elem) || elem == static_cast<Elem>('_');
	}

	template<typename Elem>
	constexpr bool IsParameterSeparator(Elem elem) noexcept
	{
		return elem == static_cast<Elem>(',');
	}
}
