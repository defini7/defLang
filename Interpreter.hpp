#pragma once

#include <deque>
#include <variant>

#include "Operator.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "Scope.hpp"

namespace def
{
	class Interpreter
	{
	public:
		Interpreter();

	public:
		std::optional<Object> Solve(const std::vector<Token>& tokens);

	private:
		void ParseIf(std::deque<Object>& solving);
		void ParseWhile(std::deque<Object>& solving);
		void ParseFor(std::deque<Object>& solving);

	private:
		Scope m_GlobalScope;

	};
}
