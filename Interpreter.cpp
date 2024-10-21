#include "Interpreter.hpp"

namespace def
{
	Interpreter::Interpreter()
	{
	}

	std::optional<Object> Interpreter::Solve(const std::vector<Token>& tokens)
	{
		// It uses Shunting yard algorithm

		std::deque<Token> holding, output;
		std::deque<Object> solving;

		Token prev(Token::Type::None);

		for (auto token : tokens)
		{
			switch (token.type)
			{
			case Token::Type::Literal_NumericBase10:
			case Token::Type::Literal_NumericBase16:
			case Token::Type::Literal_NumericBase2:
			case Token::Type::Literal_String:
			case Token::Type::Literal_Boolean:
			case Token::Type::Keyword:
			case Token::Type::Symbol:
				output.push_back(token);
				break;

			case Token::Type::Operator:
			{
				Operator op = Parser::s_Operators[token.value];

				// Check for an unary operator
				if (token.value == "+" || token.value == "-")
				{
					std::list<Token::Type> excluded =
					{
						Token::Type::Literal_NumericBase16,
						Token::Type::Literal_NumericBase10,
						Token::Type::Literal_NumericBase2,
						Token::Type::Literal_String,
						Token::Type::Symbol,
						Token::Type::Parenthesis_Close
					};

					bool notExcluded = std::find(excluded.begin(), excluded.end(), prev.type) == excluded.end();

					if (notExcluded || prev.type == Token::Type::None)
						token.value = "u" + token.value;
				}

				// Drain the stack out to the output stack until there's nothing to take or
				// the precedence of the current token is less than the precedence of the top-stack token
				while (!holding.empty() && holding.back().type != Token::Type::Parenthesis_Open && op.precedence <= Parser::s_Operators[holding.back().value].precedence)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				// only then append current token to the holding stack
				holding.push_back(token);
			}
			break;

			case Token::Type::Parenthesis_Open:
				holding.push_back(token);
				break;

			case Token::Type::Parenthesis_Close:
			{
				// Drain the holding stack out until an open parenthesis
				while (holding.back().type != Token::Type::Parenthesis_Open)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				// And remove the parenthesis by itself
				holding.pop_back();
			}
			break;

			}

			prev = token;
		}

		// Drain out the holding stack at the end
		while (!holding.empty())
		{
			output.push_back(holding.back());
			holding.pop_back();
		}

		putchar('\n');

		for (const auto& token : output)
			printf("%s\n", token.ToString().c_str());

		for (const auto& token : output)
		{
			switch (token.type)
			{
			case Token::Type::Literal_NumericBase10: solving.push_back(Numeric{ std::stold(token.value) }); break;
			case Token::Type::Literal_NumericBase16: solving.push_back(Numeric{ (long double)std::stoll(token.value, nullptr, 16) }); break;
			case Token::Type::Literal_NumericBase2:  solving.push_back(Numeric{ (long double)std::stoll(token.value, nullptr, 2) }); break;

			case Token::Type::Literal_Boolean:
				solving.push_back(Boolean{ token.value == "true" });
				break;

			case Token::Type::Literal_String:
				solving.push_back(String{ token.value });
				break;

			case Token::Type::Symbol:
				solving.push_back(Symbol{ token.value });
				break;

			case Token::Type::Keyword:
			{
				const auto& keyword = Parser::s_Keywords[token.value];

				switch (keyword.type)
				{
				case Keyword::Type::If: ParseIf(solving); break;
				case Keyword::Type::While: ParseWhile(solving); break;
				case Keyword::Type::For: ParseFor(solving); break;
				}
			}
			break;

			case Token::Type::Operator:
			{
				const auto& op = Parser::s_Operators[token.value];

				std::vector<Object> arguments(op.arguments);

				// Save all operator arguments if there are enough on the stack
				if (solving.size() < op.arguments)
					throw InterpreterException("Not enough arguments for the operator: " + token.value);

				for (auto& arg : arguments)
				{
					arg = solving.back();
					solving.pop_back();
				}

				Object object;

#define holds std::holds_alternative

				auto unwrap_value = [&]<class T>(size_t index, const std::string& error = "")
					{
						if (holds<Symbol>(arguments[index]))
						{
							std::string name = std::get<Symbol>(arguments[index]).value;

							// Let's check if a variable with the given name exists
							const auto variable = m_GlobalScope.Get(name);

							if (variable)
							{
								// The variable exists...

								const auto value = variable.value().get();

								if (!holds<T>(value))
								{
									// ... but it doesn't have type that we want
									throw InterpreterException(error);
								}

								// ... and it's of the right type so return it
								return std::get<T>(value).value;
							}

							// Couldn't find variable so assume it was an invalid symbol
							throw InterpreterException("Unexpected symbol: " + name);
						}

						if (!holds<T>(arguments[index]))
							throw InterpreterException(error);

						return std::get<T>(arguments[index]).value;
					};

#define unwrap_value(type, index, error) unwrap_value.template operator()<type>(index, error)

				switch (op.arguments)
				{
				case 1:
				{
					// Handle unary operators
					const auto number = unwrap_value(Numeric, 0, "Can't apply unary operator to the non-numeric value");

					switch (op.type)
					{
					case Operator::Type::Subtraction: object = Numeric{ -number }; break;
					case Operator::Type::Addition:    object = Numeric{ +number }; break;
					}
				}
				break;

				case 2:
				{
					// Handle binary operators

					if (holds<String>(arguments[1]))
					{
						// You can concatenate a string with another string

						const auto lhs = unwrap_value(String, 1, "");

						if (op.type != Operator::Type::Addition)
							throw InterpreterException("Can perform only concatenation (+) with strings: " + lhs);

						const auto rhs = unwrap_value(String, 0, "Can only concatenate a string with another string: " + lhs);

						object = String { lhs + rhs };
					}
					else
					{
						switch (op.type)
						{
						case Operator::Type::Equals:
						{
							if (arguments[1].index() != arguments[0].index())
								throw InterpreterException("Can't compare values of different types");

							auto check_types = [&]<class T>(const std::string& name)
							{
								if (holds<T>(arguments[1]))
								{
									const auto lhs = unwrap_value(T, 1, "");
									const auto rhs = unwrap_value(T, 0, "Can only compare a " + name + " with another " + name);

									object = Boolean{ lhs == rhs };
									return true;
								}

								return false;
							};

							if (check_types.template operator()<Numeric>("number"));
							else if (check_types.template operator()<String>("string"));
							else if (check_types.template operator()<Boolean>("boolean"));
							else
								throw InterpreterException("Can't compare 2 values");
						}
						break;

						case Operator::Type::Assign:
						{
							if (!holds<Symbol>(arguments[1]))
								throw InterpreterException("Can't create a variable with an invalid name");

							m_GlobalScope.Assign(
								std::get<Symbol>(arguments[1]).value,
								arguments[0]
							);

							object = arguments[0];
						}
						break;

						default:
						{
							const auto lhs = unwrap_value(Numeric, 1, "You must have numeric values to perform arithmetic operations");
							const auto rhs = unwrap_value(Numeric, 0, "You must have numeric values to perform arithmetic operations");

							switch (op.type)
							{
							case Operator::Type::Subtraction:    object = Numeric{ lhs - rhs };  break;
							case Operator::Type::Addition:       object = Numeric{ lhs + rhs };  break;
							case Operator::Type::Multiplication: object = Numeric{ lhs * rhs };  break;
							case Operator::Type::Division:       object = Numeric{ lhs / rhs };  break;
							}
						}

						};
					}
				}
				break;

				}

				solving.push_back(object);
			}
			break;

			}

#undef unwrap_value
#undef holds
		}

		// Just for now we only have double as a result

		if (!solving.empty())
			return solving.back();

		return std::nullopt;
	}

	void Interpreter::ParseIf(std::deque<Object>& solving)
	{
	}

	void Interpreter::ParseWhile(std::deque<Object>& solving)
	{
	}

	void Interpreter::ParseFor(std::deque<Object>& solving)
	{
	}
}
