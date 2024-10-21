#pragma once

#include <string>

namespace def
{
	class Token
	{
	public:
		enum class Type
		{
			None,
			Literal_NumericBaseUnknown,
			Literal_NumericBase16,
			Literal_NumericBase10,
			Literal_NumericBase2,
			Literal_String,
			Literal_Boolean,
			Keyword,
			Symbol,
			Comma,
			Semicolon,
			Operator,
			Parenthesis_Open,
			Parenthesis_Close
		};

	public:
		Token() = default;
		Token(Type type, const std::string& value = "");

		std::string ToString() const;

	public:
		Type type = Type::None;
		std::string value;

	};
}
