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
	constexpr auto Quotes = Create("'\"");
}

namespace error
{
	struct Exception : std::exception
	{
		Exception(const std::string& message)
		{
			m_Message = message;
		}

		const char* what() const override
		{
			return m_Message.c_str();
		}

	private:
		std::string m_Message;
	};

	struct Parser : Exception
	{
		Parser(const std::string& message) : Exception("[Parse Error] " + message) {}
	};

	struct Interpreter : Exception
	{
		Interpreter(const std::string& message) : Exception("[Interpret Error] " + message) {}
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

	static uint8_t MAX_PRECEDENCE;
};

uint8_t Operator::MAX_PRECEDENCE = std::numeric_limits<uint8_t>::max();

struct Token
{
	enum class Type
	{
		None,
		Literal_NumericBaseUnknown,
		Literal_NumericBase16,
		Literal_NumericBase10,
		Literal_NumericBase2,
		Literal_String,
		Symbol,
		Comma,
		Semicolon,
		Operator,
		Parenthesis_Open,
		Parenthesis_Close
	} type = Type::None;

	std::string value;

	std::string str() const
	{
		std::string s;

		switch (type)
		{
		case Type::Literal_NumericBase16:  s = "[Literal, Numeric 16 ] "; break;
		case Type::Literal_NumericBase10:  s = "[Literal, Numeric 10 ] "; break;
		case Type::Literal_NumericBase2:   s = "[Literal, Numeric 2  ] "; break;
		case Type::Literal_String:         s = "[Literal, String     ] "; break;
		case Type::Symbol:			       s = "[Symbol              ] "; break;
		case Type::Comma:			       s = "[Comma               ] "; break;
		case Type::Semicolon:			   s = "[Semicolon           ] "; break;
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
		Literal_String,
		Operator,
		Symbol
	};

	void Tokenise(std::string_view input, std::vector<Token>& tokens)
	{
		State stateNow = State::NewToken;
		State stateNext = State::NewToken;

		Token token;

		auto now = input.begin();

		// If they remain 0 at the end then ok
		int parenthesesBalancer = 0;
		int quotesBalancer = 0;

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

					// Check for base
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

				// Check for string literal
				else if (guard::Quotes[*now])
				{
					token = { Token::Type::Literal_String, std::string(1, *now) };
					stateNext = State::Literal_String;

					quotesBalancer++;
				}

				// Check for symbol (abc123, something_with_underscore)
				else if (guard::Symbols[*now])
				{
					token = { Token::Type::Symbol, std::string(1, *now) };
					stateNext = State::Symbol;
				}

				// Check for all kinds of open parentheses
				else if (guard::ParenthesesOpen[*now])
				{
					token = { Token::Type::Parenthesis_Open, std::string(1, *now) };
					stateNext = State::CompleteToken;

					parenthesesBalancer++;
				}

				// Check for all kinds of close parentheses
				else if (guard::ParenthesesClose[*now])
				{
					token = { Token::Type::Parenthesis_Close, std::string(1, *now) };
					stateNext = State::CompleteToken;

					parenthesesBalancer--;
				}

				else if (*now == ',')
				{
					token = { Token::Type::Comma, std::string(1, *now) };
					stateNext = State::CompleteToken;
				}

				else if (*now == ';')
				{
					token = { Token::Type::Semicolon, std::string(1, *now) };
					stateNext = State::CompleteToken;
				}

				else
					throw error::Parser(std::string("Unexpected character: ") + *now);

				now++;
			}
			else
			{
				// Perform something based on the current state

				switch (stateNow)
				{
				case State::Literal_NumericBase10:
				{
					// Check for continuation of numeric literal
					if (guard::Digits[*now])
					{
						token.value += *now;
						stateNext = State::Literal_NumericBase10;
						now++;
					}
					else
					{
						// Something has occured in the number (e.g. 531abc14124)
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
						// Determine base of numeric literal

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
					// Read hexadecimal 

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
					// Read binary

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

				case State::Literal_String:
				{
					// Read string

					if (guard::Quotes[*now])
					{
						quotesBalancer--;
						stateNext = State::CompleteToken;
					}
					else
					{
						token.value += *now;
						stateNext = State::Literal_String;
						now++;
					}
				}
				break;

				case State::Operator:
				{
					if (guard::Operators[*now])
					{
						// If we found an operator then continue searching for a longer operator
						if (s_Operators.contains(token.value + *now))
						{
							token.value += *now;
							stateNext = State::Operator;

							now++;
						}
						else
						{
							// If we don't have operator with the currently appended character then
							// proceed with the current operator
							if (s_Operators.contains(token.value))
								stateNext = State::CompleteToken;
							else
							{
								// If on the current stage we still can't find an operator
								// then probably the operator is not completed yet
								// so continue appending characters

								token.value += *now;
								stateNext = State::Operator;

								now++;
							}
						}
					}
					else
					{
						// If current character is not a part of the operator characters
						// and current text is a valid operator say that it's done
						if (s_Operators.contains(token.value))
							stateNext = State::CompleteToken;
						else
							throw error::Parser("Invalid operator was found: " + token.value);
					}
				}
				break;

				case State::Symbol:
				{
					// Note: we treat all invalid symbols (e.g. 123abc) in the Literal_Numeric state
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

		if (quotesBalancer != 0)
			throw error::Parser("Quotes were not balanced");

		// Drain out a last token
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

	// Unary operators (in that way they are easier to handle)
	{"u-", { Operator::Type::Subtraction, Operator::MAX_PRECEDENCE, 1 } },
	{"u+", { Operator::Type::Addition, Operator::MAX_PRECEDENCE, 1 } },
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
			// Convert all numeric literals of the bases other than 10 into base10
			// so it becomes easier to work with them later

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

				// Check for an unary operator
				if (token.value == "+" || token.value == "-")
				{
					if ((prev.type != Token::Type::Literal_NumericBase10 && prev.type != Token::Type::Parenthesis_Close) || prev.type == Token::Type::None)
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

		while (!output.empty())
		{
			const auto token = output.front();

			switch (token.type)
			{
			case Token::Type::Literal_NumericBase10:
			{
				// Just push every literal to the solving stack
				solving.push_back(token);
				output.pop_front();
			}
			break;

			case Token::Type::Operator:
			{
				const auto& op = Parser::s_Operators[token.value];

				std::vector<Token> arguments(op.arguments);

				// Save all operator arguments if there are enough on the stack
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

				Token token;

				switch (op.arguments)
				{
				case 1:
				{
					// Handle unary operators

					double val = std::stod(arguments[0].value);

					token.type = Token::Type::Literal_NumericBase10;

					switch (op.type)
					{
					case Operator::Type::Subtraction: token.value = std::to_string(-val); break;
					case Operator::Type::Addition:    token.value = std::to_string(+val); break;
					}
				}
				break;

				case 2:
				{
					// Handle binary operators

					double lhs = std::stod(arguments[1].value);
					double rhs = std::stod(arguments[0].value);

					token.type = Token::Type::Literal_NumericBase10;

					switch (op.type)
					{
					case Operator::Type::Subtraction:    token.value = std::to_string(lhs - rhs); break;
					case Operator::Type::Addition:       token.value = std::to_string(lhs + rhs); break;
					case Operator::Type::Multiplication: token.value = std::to_string(lhs * rhs); break;
					case Operator::Type::Division:       token.value = std::to_string(lhs / rhs); break;
					}
				}
				break;

				}

				solving.push_back(token);
				output.pop_front();
			}
			break;

			}
		}

		// Just for now we only have double as a result
		return std::stod(solving.back().value);
	}
};

int main()
{
	Parser parser;
	Interpreter interpreter;

	std::string input;

	// Simple REPL
	do
	{
		std::cout << "> ";
		std::getline(std::cin, input);

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
	}
	while (input != "quit");

	return 0;
}
