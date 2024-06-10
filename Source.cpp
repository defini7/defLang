#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <deque>

namespace guard
{
	static constexpr std::array<bool, 256> Create(std::string_view availableCharacters)
	{
		std::array<bool, 256> chars{ false };

		for (auto c : availableCharacters)
			chars[c] = true;

		return chars;
	}

	constexpr auto Digits = Create(".0123456789");
	constexpr auto Whitespaces = Create(" \t\n\r\v");
	constexpr auto Symbols = Create("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789_.");
	constexpr auto Operators = Create("+-*/");
	constexpr auto ParenthesesOpen = Create("([{");
	constexpr auto ParenthesesClose = Create(")]}");
}

namespace error
{
	struct Parser : std::exception
	{
		Parser(const std::string& message)
		{
			Message = "[Parse Error] " + message;
		}

		const char* what() const override
		{
			return Message.c_str();
		}

		std::string Message;
	};

	struct Interpreter : std::exception
	{
		Interpreter(const std::string& message)
		{
			this->message = "[Interpret Error] " + message;
		}

		const char* what() const override
		{
			return message.c_str();
		}

		std::string message;
	};
}

class Parser
{
public:
	Parser() {}

	struct Operator
	{
		enum class Type
		{
			Subtraction,
			Addition,
			Multiplication,
			Division
		} type;

		uint8_t precedence;
		uint8_t arguments;
	};

	static std::unordered_map<std::string, Operator> s_Operators;

public:
	enum class State
	{
		NewToken,
		CompleteToken,
		Literal_Numeric,
		Operator,
		Symbol
	};

	struct Token
	{
		enum class Type
		{
			Literal_Numeric,
			Symbol,
			Operator,
			Parenthesis_Open,
			Parenthesis_Close
		} type;

		std::string value;

		std::string str() const
		{
			std::string s;

			switch (type)
			{
			case Type::Literal_Numeric:  s = "[Literal, Numeric  ] "; break;
			case Type::Symbol:			 s = "[Symbol            ] "; break;
			case Type::Operator:		 s = "[Operator          ] "; break;
			case Type::Parenthesis_Open: s = "[Parenthesis, Open ] "; break;
			case Type::Parenthesis_Close:s = "[Parenthesis, Close] "; break;
			}

			return s + value;
		}
	};

	void Tokenise(std::string_view input, std::vector<Token>& tokens)
	{
		State stateNow = State::NewToken;
		State stateNext = State::NewToken;

		Token token;

		auto now = input.begin();

		int parenthesesBalancer = 0;

		while (now != input.end())
		{
			// FDA - First Digit Analysis

			// Skip whitespace
			if (guard::Whitespaces[*now])
			{
				stateNext = State::NewToken;
				now++;
				continue;
			}

			if (stateNow == State::NewToken)
			{
				// Check for digit
				if (guard::Digits[*now])
				{
					token = { Token::Type::Literal_Numeric, std::string(1, *now) };
					stateNext = State::Literal_Numeric;
				}

				// Check for operator
				else if (guard::Operators[*now])
				{
					token = { Token::Type::Operator, std::string(1, *now) };
					stateNext = State::Operator;
				}

				else if (guard::Symbols[*now])
				{
					token = { Token::Type::Symbol, std::string(1, *now) };
					stateNext = State::Symbol;
				}

				else if (guard::ParenthesesOpen[*now])
				{
					token = { Token::Type::Parenthesis_Open, std::string(1, *now) };
					stateNext = State::CompleteToken;

					parenthesesBalancer++;
				}

				else if (guard::ParenthesesClose[*now])
				{
					token = { Token::Type::Parenthesis_Close, std::string(1, *now) };
					stateNext = State::CompleteToken;

					parenthesesBalancer--;
				}

				else
					throw error::Parser(std::string("Unexpected character: ") + *now);

				now++;
			}
			else
			{
				switch (stateNow)
				{
				case State::Literal_Numeric:
				{
					if (guard::Digits[*now])
					{
						token.value += *now;
						stateNext = State::Literal_Numeric;
						now++;
					}
					else
					{
						if (guard::Symbols[*now])
							throw error::Parser("Invalid numeric literal or symbol");

						stateNext = State::CompleteToken;
					}
				}
				break;

				case State::Operator:
				{
					if (guard::Operators[*now])
					{
						if (s_Operators.contains(token.value + *now))
						{
							token.value += *now;
							stateNext = State::Operator;

							now++;
						}
						else
						{
							if (s_Operators.contains(token.value))
								stateNext = State::CompleteToken;
							else
							{
								token.value += *now;
								stateNext = State::Operator;

								now++;
							}
						}
					}
					else
					{
						if (s_Operators.contains(token.value))
							stateNext = State::CompleteToken;
						else
							throw error::Parser("Invalid operator was found: " + token.value);
					}
				}
				break;

				case State::Symbol:
				{
					if (guard::Symbols[*now] || guard::Digits[*now])
					{
						token.value += *now;
						stateNext = State::Symbol;

						now++;
					}
					else
						stateNext = State::CompleteToken;
				}
				break;

				case State::CompleteToken:
				{
					stateNext = State::NewToken;
					tokens.push_back(token);
				}
				break;

				}
			}

			stateNow = stateNext;
		}

		if (parenthesesBalancer != 0)
			throw error::Parser("Parenthesis " + token.value + " was not balanced");

		if (!token.value.empty())
			tokens.push_back(token);
	}
};

std::unordered_map<std::string, Parser::Operator> Parser::s_Operators =
{
	{"-", { Parser::Operator::Type::Subtraction, 1, 2 } },
	{"+", { Parser::Operator::Type::Addition, 1, 2 } },
	{"*", { Parser::Operator::Type::Multiplication, 2, 2 } },
	{"/", { Parser::Operator::Type::Division, 2, 2 } }
};

class Interpreter
{
public:
	Interpreter() {}

public:
	double Solve(const std::vector<Parser::Token>& tokens)
	{
		// It uses Shunting yard algorithm

		std::deque<Parser::Token> holding, output, solving;

		for (const auto& token : tokens)
		{
			switch (token.type)
			{
			case Parser::Token::Type::Literal_Numeric: output.push_back(token); break;
			case Parser::Token::Type::Operator:
			{
				const auto& op = Parser::s_Operators[token.value];

				while (!holding.empty() && holding.back().type != Parser::Token::Type::Parenthesis_Open && op.precedence <= Parser::s_Operators[holding.back().value].precedence)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				holding.push_back(token);
			}
			break;

			case Parser::Token::Type::Parenthesis_Open:
				holding.push_back(token);
			break;

			case Parser::Token::Type::Parenthesis_Close:
			{
				while (holding.back().type != Parser::Token::Type::Parenthesis_Open)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				holding.pop_back();
			}
			break;

			}
		}

		while (!holding.empty())
		{
			output.push_back(holding.back());
			holding.pop_back();
		}

		std::cout << std::endl;

		for (const auto& token : output)
			std::cout << token.str() << std::endl;
		
		while (!output.empty())
		{
			const auto token = output.front();

			switch (token.type)
			{
			case Parser::Token::Type::Literal_Numeric:
			{
				solving.push_back(token);
				output.pop_front();
			}
			break;

			case Parser::Token::Type::Operator:
			{
				const auto& op = Parser::s_Operators[token.value];

				std::vector<Parser::Token> arguments(op.arguments);

				if (solving.size() >= op.arguments)
				{
					for (auto& arg : arguments)
					{
						arg = solving.back();
						solving.pop_back();
					}
				}
				else
					throw error::Interpreter("Not enough arguments for the operator: " + token.value);

				if (op.arguments == 2)
				{
					double lhs = std::stod(arguments[1].value);
					double rhs = std::stod(arguments[0].value);

					switch (op.type)
					{
					case Parser::Operator::Type::Subtraction:    solving.push_back({ Parser::Token::Type::Literal_Numeric, std::to_string(lhs - rhs) }); break;
					case Parser::Operator::Type::Addition:       solving.push_back({ Parser::Token::Type::Literal_Numeric, std::to_string(lhs + rhs) }); break;
					case Parser::Operator::Type::Multiplication: solving.push_back({ Parser::Token::Type::Literal_Numeric, std::to_string(lhs * rhs) }); break;
					case Parser::Operator::Type::Division:       solving.push_back({ Parser::Token::Type::Literal_Numeric, std::to_string(lhs / rhs) }); break;
					}
				}

				output.pop_front();
			}
			break;

			}
		}

		return std::stod(solving.back().value);
	}
};

int main()
{
	std::string input = "0-12000-(16*4)-20";

	Parser parser;
	Interpreter interpreter;

	std::cout << input << std::endl << std::endl;

	try
	{
		std::vector<Parser::Token> tokens;
		parser.Tokenise(input, tokens);

		for (const auto& token : tokens)
			std::cout << token.str() << std::endl;

		double result = interpreter.Solve(tokens);

		std::cout << result << std::endl;
	}
	catch (const error::Parser& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
