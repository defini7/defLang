#pragma once

#include <unordered_map>
#include <string>
#include <list>

#include "Operator.hpp"
#include "Token.hpp"
#include "Guard.hpp"
#include "Exception.hpp"
#include "Keyword.hpp"

namespace def
{
	class Parser
	{
	public:
		Parser();

	public:
		enum class State
		{
			NewToken,
			CompleteToken,
			Literal_NumericBaseUnknown,
			Literal_NumericBase16,
			Literal_NumericBase10,
			Literal_NumericBase8,
			Literal_NumericBase2,
			Literal_String,
			Operator,
			Symbol
		};

		void Tokenise(std::string_view input, std::vector<Token>& tokens);

	public:
		static std::unordered_map<std::string, Operator> s_Operators;
		static std::unordered_map<std::string, Keyword> s_Keywords;

	};
}
