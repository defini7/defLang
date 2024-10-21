#include "Token.hpp"

namespace def
{
	Token::Token(Type type, const std::string& value) : type(type), value(value)
	{

	}

	std::string Token::ToString() const
	{
		std::string tag;

		switch (type)
		{
		case Type::Literal_NumericBase16:  tag = "[Literal, Numeric 16 ] "; break;
		case Type::Literal_NumericBase10:  tag = "[Literal, Numeric 10 ] "; break;
		case Type::Literal_NumericBase2:   tag = "[Literal, Numeric 2  ] "; break;
		case Type::Literal_String:         tag = "[Literal, String     ] "; break;
		case Type::Keyword:			       tag = "[Keyword             ] "; break;
		case Type::Symbol:			       tag = "[Symbol              ] "; break;
		case Type::Comma:			       tag = "[Comma               ] "; break;
		case Type::Semicolon:			   tag = "[Semicolon           ] "; break;
		case Type::Operator:		       tag = "[Operator            ] "; break;
		case Type::Parenthesis_Open:       tag = "[Parenthesis, Open   ] "; break;
		case Type::Parenthesis_Close:      tag = "[Parenthesis, Close  ] "; break;
		}

		return tag + value;
	}
}
