#include "StringUtil.h"
#include <cmath>

namespace util::string
{
	template<typename Integral, typename Elem, typename Traits, typename Alloc>
		requires(std::is_integral_v<Integral>)
	bool IToS(Integral integral, std::basic_string<Elem, Traits, Alloc>& outString, uint8_t radix, bool lowercase)
	{
		// Check if the radix is valid.
		if (radix < 2 || radix > 36)
			return false;

		using String = std::basic_string<Elem, Traits, Alloc>;
		using Size = String::size_type;

		// Check if integral is negative.
		bool negative;
		Integral absIntegral;
		if constexpr (std::is_signed_v<Integral>)
		{
			negative = integral < static_cast<Integral>(0);
			absIntegral = negative ? -integral : integral;
		}
		else
		{
			negative = false;
			absIntegral = integral;
		}

		// Clear the string, in case it already has contents.
		outString.clear();

		// Check if abs(integral) == 0 because log(0) is undefined.
		// Check if abs(integral) == 1 because log(1) is 0, so length would be 0 instead of 1.
		if (absIntegral < static_cast<Integral>(2))
		{
			if constexpr (std::is_signed_v<Integral>)
			{
				if (negative)
				{
					outString.resize(2);
					outString.front() = static_cast<Elem>('-');
				}
				else
					outString.resize(1);
			}
			else
				outString.resize(1);
			outString.back() = static_cast<Elem>(static_cast<Elem>('0') + absIntegral);
			return true;
		}

		// Get the number of characters integral will result in.
		Size length = negative + static_cast<Size>(std::ceil(std::log(absIntegral) / std::log(radix)));

		// Make the output string have enough memory to hold the result.
		outString.resize(length);

		if (negative)
			outString.front() = static_cast<Elem>('-');

		// Get the digit set based on case.
		static constexpr char base36DigitsUpper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		static constexpr char base36DigitsLower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
		const char* digits = lowercase ? base36DigitsLower : base36DigitsUpper;

		// Start at the last digit and work backward.
		auto it = outString.end();
		do
		{
			// Insert the digit.
			*--it = static_cast<Elem>(digits[absIntegral % radix]);

			// Prepare for the next digit.
			absIntegral /= radix;
		}
		while (absIntegral);

		return true;
	}

	template<typename Integral, typename Elem, typename Traits>
		requires(std::is_integral_v<Integral>)
	bool SToI(std::basic_string_view<Elem, Traits> stringView, Integral& outIntegral, uint8_t radix) noexcept
	{
		// Check if the radix or string are trivially invalid.
		if (radix < 2 || radix > 36 || stringView.empty())
			return false;

		// Check if negative.
		bool negative = stringView.front() == '-';

		using Size = std::basic_string_view<Elem, Traits>::size_type;

		// Check if outIntegral is unsigned, and thus can't be negative.
		if constexpr (std::is_unsigned_v<Integral>)
		{
			if (negative)
				return false;
		}
		// If outIntegral is signed and all stringView has is a negative sign, it is invalid.
		else if (negative && stringView.size() == static_cast<Size>(1))
			return false;

		// Get the maximum number of characters outIntegral can be represented by.
		Size maxSize = negative + static_cast<Size>(std::ceil((sizeof(Integral) * 8) * std::log(2) / std::log(radix)));

		// Check if the string is too long to represent the integral.
		if (stringView.size() > maxSize)
			return false;

		outIntegral = static_cast<Integral>(0);

		// Get the starting digit scale.
		Integral digitScale = static_cast<Integral>(1);
		if constexpr (!std::is_unsigned_v<Integral>) // this is just to avoid compiler warnings.
		{
			if (negative)
				digitScale = -digitScale;
		}

		for (auto it = stringView.rbegin(); it != stringView.rend() - negative; ++it)
		{
			Elem elem = *it;
			Integral digit = 0;

			// Get the digit value.
			if (radix <= 10)
			{
				if (static_cast<Elem>('0') <= elem && elem <= static_cast<Elem>('0' - 1 + radix))
					digit = static_cast<Integral>(elem - static_cast<Elem>('0'));
				else // Elem is invalid.
					return false;
			}
			else if (static_cast<Elem>('0') <= elem && elem <= static_cast<Elem>('9'))
				digit = static_cast<Integral>(elem - static_cast<Elem>('0'));
			else if (static_cast<Elem>('A') <= elem && elem <= static_cast<Elem>('A' - 1 - 10 + radix))
				digit = static_cast<Integral>(elem - static_cast<Elem>('A'));
			else if (static_cast<Elem>('a') <= elem && elem <= static_cast<Elem>('a' - 1 - 10 + radix))
				digit = static_cast<Integral>(elem - static_cast<Elem>('a'));
			else // Elem is invalid.
				return false;

			outIntegral += digit * digitScale;
			digitScale *= radix;
		}

		return true;
	}

	template<typename Integral, typename Elem, typename Traits, typename Alloc>
		requires(std::is_integral_v<Integral>)
	bool SToI(const std::basic_string<Elem, Traits, Alloc>& string, Integral& outIntegral, uint8_t radix) noexcept
	{
		return SToI(static_cast<std::basic_string_view<Elem, Traits>>(string), outIntegral, radix);
	}

	template<typename Predicate, typename Elem, typename Traits>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	void FindFirstNotOf(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))))
	{
		while (predicate(elem) && ++index < stringView.size())
			elem = stringView[index];
	}

	template<typename Predicate, typename Elem, typename Traits>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	void FindFirstOf(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))))
	{
		static Predicate s_Predicate;
		s_Predicate = predicate;
		FindFirstNotOf(stringView, index, elem, [](Elem elem2) noexcept(noexcept(predicate(static_cast<Elem>(elem)))) { return !s_Predicate(elem2); });
	}

	template<typename Predicate, typename Elem, typename Traits>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	bool FindFirstNotOfPastQuote(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))))
	{
		static Predicate s_Predicate;
		s_Predicate = predicate;
		return FindFirstOfPastQuote(stringView, index, elem, [](Elem elem2) noexcept(noexcept(predicate(static_cast<Elem>(elem)))) { return !s_Predicate(elem2); });
	}

	template<typename Predicate, typename Elem, typename Traits>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	bool FindFirstOfPastQuote(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))))
	{
		if (!stringView.empty())
		{
			if (size_t i = stringView.find('"'); i == std::basic_string_view<Elem, Traits>::npos)
			{
				FindFirstOf(stringView, index, elem, predicate);
				return true;
			}
			else
			{
				char c;
				bool quoted = false;
				bool escaped = false;
				while (index < stringView.size())
				{
					c = stringView[index];
					if (quoted)
					{
						if (c == '"' && !escaped)
							quoted = false;
						else
							escaped = c == '\\' && !escaped;
					}
					else if (c == '"')
						quoted = true;
					else if (predicate(c))
						return true;
					index++;
				}
				if (quoted || escaped)
					return false;
			}
		}

		index = stringView.size();
		return true;
	}

	template<typename Elem>
	constexpr bool IsBlank(Elem elem) noexcept
	{
		return elem == static_cast<Elem>(' ') || elem == static_cast<Elem>('\t');
	}

	template<typename Elem>
	constexpr bool IsLineEnding(Elem elem) noexcept
	{
		return elem == static_cast<Elem>('\n') || elem == static_cast<Elem>('\r');
	}

	template<typename Elem>
	constexpr bool IsSpace(Elem elem) noexcept
	{
		return IsBlank(elem) || IsLineEnding(elem) ||
			elem == static_cast<Elem>('\f') || elem == static_cast<Elem>('\v');
	}

	template<typename Elem>
	constexpr bool IsBinaryDigit(Elem elem) noexcept
	{
		return elem == static_cast<Elem>('0') || elem == static_cast<Elem>('1');
	}

	template<typename Elem>
	constexpr bool IsDecimalDigit(Elem elem) noexcept
	{
		return static_cast<Elem>('0') <= elem && elem <= static_cast<Elem>('9');
	}

	template<typename Elem>
	constexpr bool IsHexadecimalDigit(Elem elem) noexcept
	{
		return IsDecimalDigit(elem) ||
			(static_cast<Elem>('A') <= elem && elem <= static_cast<Elem>('F')) ||
			(static_cast<Elem>('a') <= elem && elem <= static_cast<Elem>('f'));
	}

	template<typename Elem>
	constexpr bool IsUpper(Elem elem) noexcept
	{
		return static_cast<Elem>('A') <= elem && elem <= static_cast<Elem>('Z');
	}

	template<typename Elem>
	constexpr bool IsLower(Elem elem) noexcept
	{
		return static_cast<Elem>('a') <= elem && elem <= static_cast<Elem>('z');
	}

	template<typename Elem>
	constexpr bool IsAlpha(Elem elem) noexcept
	{
		return IsUpper(elem) || IsLower(elem);
	}

	template<typename Elem>
	constexpr bool IsAlphanumeric(Elem elem) noexcept
	{
		return IsAlpha(elem) || IsDecimalDigit(elem);
	}

	template<typename Elem>
	constexpr bool IsWord(Elem elem) noexcept
	{
		return IsAlphanumeric(elem) || elem == static_cast<Elem>('_');
	}

	template<typename Elem>
	constexpr Elem ToLower(Elem elem) noexcept
	{
		return IsUpper(elem) ? elem + static_cast<Elem>('a' - 'A') : elem;
	}

	template<typename Elem>
	constexpr Elem ToUpper(Elem elem) noexcept
	{
		return IsLower(elem) ? elem - static_cast<Elem>('a' - 'A') : elem;
	}
}
