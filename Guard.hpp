#pragma once

#include <array>
#include <string>

namespace def
{
	namespace guard
	{
		static constexpr std::array<bool, 256> Create(const std::string& availableCharacters)
		{
			std::array<bool, 256> chars{ false };

			for (auto c : availableCharacters)
				chars[c] = true;

			return chars;
		}

		constexpr auto Digits = Create(".0123456789");
		constexpr auto HexDigits = Create("0123456789ABCDEFabcdef");
		constexpr auto OctDigits = Create("01234567");
		constexpr auto BinDigits = Create("01");
		constexpr auto Prefixes = Create("xob");
		constexpr auto Whitespaces = Create(" \t\n\r\v");
		constexpr auto Symbols = Create("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789_.");
		constexpr auto Operators = Create("+-*/=");
		constexpr auto ParenthesesOpen = Create("([{");
		constexpr auto ParenthesesClose = Create(")]}");
		constexpr auto Quotes = Create("'\"");
	}
}