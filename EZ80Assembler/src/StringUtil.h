#pragma once

#include <string_view>

namespace util::string
{
	// Converts an integral x in the given radix(aka base) to a string, either using uppercase or lowercase letters.
	// Radix is in the range [2, 37), anything outside this range will be considered invalid.
	// Returns if the conversion was successful.
	template<typename Integral, typename Elem = char, typename Traits = std::char_traits<Elem>, typename Alloc = std::allocator<Elem>>
		requires(std::is_integral_v<Integral>)
	bool IToS(Integral integral, std::basic_string<Elem, Traits, Alloc>& outString, uint8_t radix = 10, bool lowercase = false);

	// Converts a string to an integral in the given radix(aka base).
	// Prepending and apending whitespace count as invalid.
	// Radix is in the range [2, 37), anything outside this range will be considered invalid.
	// Returns if the conversion was successful. If not, outIntegral may or may not modified.
	template<typename Integral, typename Elem = char, typename Traits = std::char_traits<Elem>>
		requires(std::is_integral_v<Integral>)
	bool SToI(std::basic_string_view<Elem, Traits> stringView, Integral& outIntegral, uint8_t radix = 10) noexcept;

	// Alias for bool SToI(std::basic_string_view<Elem, Traits> stringView, Integral& outIntegral, uint8_t radix).
	// Because of templates, the implicit std::basic_string_view cast doesn't happen, so this is doing that manually.
	template<typename Integral, typename Elem = char, typename Traits = std::char_traits<Elem>, typename Alloc = std::allocator<Elem>>
		requires(std::is_integral_v<Integral>)
	bool SToI(const std::basic_string<Elem, Traits, Alloc>& string, Integral& outIntegral, uint8_t radix = 10) noexcept;

	template<typename Predicate, typename Elem = char, typename Traits = std::char_traits<Elem>>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	void FindFirstNotOf(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))));

	template<typename Predicate, typename Elem = char, typename Traits = std::char_traits<Elem>>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	void FindFirstOf(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))));

	// Returns false if the quoted text was not properly ended, true otherwise.
	template<typename Predicate, typename Elem = char, typename Traits = std::char_traits<Elem>>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	bool FindFirstNotOfPastQuote(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))));

	// Returns false if the quoted text was not properly ended, true otherwise.
	template<typename Predicate, typename Elem = char, typename Traits = std::char_traits<Elem>>
		requires(std::is_convertible_v<Predicate, bool(*)(Elem)>)
	bool FindFirstOfPastQuote(std::basic_string_view<Elem, Traits> stringView, typename std::basic_string_view<Elem, Traits>::size_type& index, Elem& elem, Predicate predicate) noexcept(noexcept(predicate(static_cast<Elem>(elem))));

	template<typename Elem = char>
	constexpr bool IsBlank(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsLineEnding(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsSpace(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsBinaryDigit(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsDecimalDigit(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsHexadecimalDigit(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsUpper(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsLower(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsAlpha(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsAlphanumeric(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr bool IsWord(Elem elem) noexcept;

	template<typename Elem = char>
	constexpr Elem ToLower(Elem elem) noexcept;
	template<typename Elem = char>
	constexpr Elem ToUpper(Elem elem) noexcept;
}

#include "StringUtil.inl"
