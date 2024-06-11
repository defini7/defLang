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
	constexpr auto HexDigits = Create("0123456789ABCDEFabcdef");
	constexpr auto OctDigits = Create("01234567");
	constexpr auto BinDigits = Create("01");
	constexpr auto Prefixes = Create("xob");
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

struct Token
{
	enum class Type
	{
		None,
		Literal_NumericBaseUnknown,
		Literal_NumericBase16,
		Literal_NumericBase10,
		Literal_NumericBase2,
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
		case Type::Literal_NumericBase16:  s = "[Literal, Numeric 16 ] "; break;
		case Type::Literal_NumericBase10:  s = "[Literal, Numeric 10 ] "; break;
		case Type::Literal_NumericBase2:   s = "[Literal, Numeric 2  ] "; break;
		case Type::Symbol:			       s = "[Symbol              ] "; break;
		case Type::Operator:		       s = "[Operator            ] "; break;
		case Type::Parenthesis_Open:       s = "[Parenthesis, Open   ] "; break;
		case Type::Parenthesis_Close:      s = "[Parenthesis, Close  ] "; break;
		}

		return s + value;
	}
};

class Parser
{
public:
	Parser() {}

	static std::unordered_map<std::string, Operator> s_Operators;

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
		Operator,
		Symbol
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
					token = { Token::Type::None, std::string(1, *now) };

					if (*now == '0')
					{
						token.type = Token::Type::Literal_NumericBaseUnknown;
						stateNext = State::Literal_NumericBaseUnknown;
					}
					else
					{
						token.type = Token::Type::Literal_NumericBase10;
						stateNext = State::Literal_NumericBase10;
					}
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
				case State::Literal_NumericBase10:
				{
					if (guard::Digits[*now])
					{
						token.value += *now;
						stateNext = State::Literal_NumericBase10;
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

				case State::Literal_NumericBaseUnknown:
				{
					if (guard::Prefixes[*now])
					{
						if (*now == 'x' || *now == 'X')
						{
							stateNext = State::Literal_NumericBase16;
							token.type = Token::Type::Literal_NumericBase16;
						}

						else if (*now == 'b' || *now == 'B')
						{
							stateNext = State::Literal_NumericBase2;
							token.type = Token::Type::Literal_NumericBase2;
						}

						now++;
					}
					else
						throw error::Parser("Unknown prefix for numeric literal");
				}
				break;

				case State::Literal_NumericBase16:
				{
					if (guard::HexDigits[*now])
					{
						token.value += *now;
						stateNext = State::Literal_NumericBase16;
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

				case State::Literal_NumericBase2:
				{
					if (guard::HexDigits[*now])
					{
						token.value += *now;
						stateNext = State::Literal_NumericBase2;
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
			throw error::Parser("Parentheses were not balanced");

		if (!token.value.empty())
			tokens.push_back(token);
	}
};

std::unordered_map<std::string, Operator> Parser::s_Operators =
{
	{"-", { Operator::Type::Subtraction, 1, 2 } },
	{"+", { Operator::Type::Addition, 1, 2 } },
	{"*", { Operator::Type::Multiplication, 2, 2 } },
	{"/", { Operator::Type::Division, 2, 2 } },
	{"u-", { Operator::Type::Subtraction, UINT8_MAX, 1 } },
	{"u+", { Operator::Type::Addition, UINT8_MAX, 1 } },
};

class Interpreter
{
public:
	Interpreter() {}

public:
	double Solve(const std::vector<Token>& tokens)
	{
		// It uses Shunting yard algorithm

		std::deque<Token> holding, output, solving;

		Token prev = { Token::Type::None };

		for (auto token : tokens)
		{
			switch (token.type)
			{
			case Token::Type::Literal_NumericBase16:
			{
				token.type = Token::Type::Literal_NumericBase10;
				token.value = std::to_string(std::stol(token.value, nullptr, 16));
			}
			break;

			case Token::Type::Literal_NumericBase2:
			{
				token.type = Token::Type::Literal_NumericBase10;
				token.value = std::to_string(std::stol(token.value, nullptr, 2));
			}
			break;

			}

			switch (token.type)
			{
			case Token::Type::Literal_NumericBase10: output.push_back(token); break;
			case Token::Type::Operator:
			{
				Operator op = Parser::s_Operators[token.value];
				
				if (token.value == "+" || token.value == "-")
				{
					if ((prev.type != Token::Type::Literal_NumericBase10 && prev.type != Token::Type::Parenthesis_Close) || prev.type == Token::Type::None)
						token.value = "u" + token.value;
				}

				while (!holding.empty() && holding.back().type != Token::Type::Parenthesis_Open && op.precedence <= Parser::s_Operators[holding.back().value].precedence)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				holding.push_back(token);
			}
			break;

			case Token::Type::Parenthesis_Open:
				holding.push_back(token);
			break;

			case Token::Type::Parenthesis_Close:
			{
				while (holding.back().type != Token::Type::Parenthesis_Open)
				{
					output.push_back(holding.back());
					holding.pop_back();
				}

				holding.pop_back();
			}
			break;

			}

			prev = token;
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
			case Token::Type::Literal_NumericBase10:
			{
				solving.push_back(token);
				output.pop_front();
			}
			break;

			case Token::Type::Operator:
			{
				const auto& op = Parser::s_Operators[token.value];

				std::vector<Token> arguments(op.arguments);

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
				
				switch (op.arguments)
				{
				case 1:
				{
					double val = std::stod(arguments[0].value);

					switch (op.type)
					{
					case Operator::Type::Subtraction:    solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(-val) }); break;
					case Operator::Type::Addition:       solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(+val) }); break;
					}
				}
				break;

				case 2:
				{
					double lhs = std::stod(arguments[1].value);
					double rhs = std::stod(arguments[0].value);

					switch (op.type)
					{
					case Operator::Type::Subtraction:    solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(lhs - rhs) }); break;
					case Operator::Type::Addition:       solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(lhs + rhs) }); break;
					case Operator::Type::Multiplication: solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(lhs * rhs) }); break;
					case Operator::Type::Division:       solving.push_back({ Token::Type::Literal_NumericBase10, std::to_string(lhs / rhs) }); break;
					}
				}
				break;

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
	std::string input = "0b0100 + 0xFF";

	Parser parser;
	Interpreter interpreter;

	std::cout << input << std::endl << std::endl;

	try
	{
		std::vector<Token> tokens;
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
