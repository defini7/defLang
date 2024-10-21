#include "Parser.hpp"

namespace def
{
	Parser::Parser()
	{

	}

	void Parser::Tokenise(std::string_view input, std::vector<Token>& tokens)
	{
		State stateNow = State::NewToken;
		State stateNext = State::NewToken;

		Token token;

		auto currentChar = input.begin();

		// If they remain 0 at the end then ok
		int parenthesesBalancer = 0;
		int quotesBalancer = 0;

		auto StartToken = [&](Token::Type type, State nextState = State::CompleteToken, bool push = true)
			{
				token.type = type;

				if (push)
					token.value.push_back(*currentChar);

				stateNext = nextState;
			};

		auto AppendChar = [&](State nextState)
			{
				token.value += *currentChar;
				stateNext = nextState;
				currentChar++;
			};

		while (currentChar != input.end())
		{
			// FDA - First Digit Analysis

			if (stateNow == State::NewToken)
			{
				// Skip whitespace
				if (guard::Whitespaces[*currentChar])
				{
					stateNext = State::NewToken;
					currentChar++;
					continue;
				}

				// Check for a digit
				if (guard::Digits[*currentChar])
				{
					// Check for a base
					if (*currentChar == '0')
						StartToken(Token::Type::Literal_NumericBaseUnknown, State::Literal_NumericBaseUnknown, false);
					else
						StartToken(Token::Type::Literal_NumericBase10, State::Literal_NumericBase10);
				}

				// Check for an operator
				else if (guard::Operators[*currentChar])
					StartToken(Token::Type::Operator, State::Operator);

				// Check for a string literal
				else if (guard::Quotes[*currentChar])
				{
					StartToken(Token::Type::Literal_String, State::Literal_String, false);
					quotesBalancer++;
				}

				// Check for a symbol (e.g. abc123, something_with_underscore)
				else if (guard::Symbols[*currentChar])
					StartToken(Token::Type::Symbol, State::Symbol);

				// Check for all kinds of open parentheses
				else if (guard::ParenthesesOpen[*currentChar])
				{
					StartToken(Token::Type::Parenthesis_Open);
					parenthesesBalancer++;
				}

				// Check for all kinds of close parentheses
				else if (guard::ParenthesesClose[*currentChar])
				{
					StartToken(Token::Type::Parenthesis_Close);
					parenthesesBalancer--;
				}

				else if (*currentChar == ',')
					StartToken(Token::Type::Comma);

				else if (*currentChar == ';')
					StartToken(Token::Type::Semicolon);

				else
					throw ParserException(std::string("Unexpected character: ") + *currentChar);

				currentChar++;
			}
			else
			{
				// Perform something based on the current state

				switch (stateNow)
				{
				case State::Literal_NumericBase10:
				{
					// Check for continuation of numeric literal
					if (guard::Digits[*currentChar])
						AppendChar(State::Literal_NumericBase10);
					else
					{
						// Something has occured in the number (e.g. 531abc14124)
						if (guard::Symbols[*currentChar])
							throw ParserException("Invalid numeric literal or symbol");

						stateNext = State::CompleteToken;
					}
				}
				break;

				case State::Literal_NumericBaseUnknown:
				{
					if (guard::Prefixes[*currentChar])
					{
						// Determine base of numeric literal

						if (*currentChar == 'x' || *currentChar == 'X')
						{
							token.type = Token::Type::Literal_NumericBase16;
							stateNext = State::Literal_NumericBase16;
						}

						else if (*currentChar == 'b' || *currentChar == 'B')
						{
							token.type = Token::Type::Literal_NumericBase2;
							stateNext = State::Literal_NumericBase2;
						}

						currentChar++;
					}
					else
						throw ParserException("Unknown prefix for numeric literal");
				}
				break;

				case State::Literal_NumericBase16:
				{
					// Read hexadecimal number

					if (guard::HexDigits[*currentChar])
						AppendChar(State::Literal_NumericBase16);
					else
					{
						if (guard::Symbols[*currentChar])
							throw ParserException("Invalid numeric literal or symbol");

						stateNext = State::CompleteToken;
					}
				}
				break;

				case State::Literal_NumericBase2:
				{
					// Read binary number

					if (guard::HexDigits[*currentChar])
						AppendChar(State::Literal_NumericBase2);
					else
					{
						if (guard::Symbols[*currentChar])
							throw ParserException("Invalid numeric literal or symbol");

						stateNext = State::CompleteToken;
					}
				}
				break;

				case State::Literal_String:
				{
					// Read string

					if (guard::Quotes[*currentChar])
					{
						quotesBalancer--;
						stateNext = State::CompleteToken;
						currentChar++;
					}
					else
						AppendChar(State::Literal_String);
				}
				break;

				case State::Operator:
				{
					if (guard::Operators[*currentChar])
					{
						// If we found an operator then continue searching for a longer operator
						if (s_Operators.contains(token.value + *currentChar))
							AppendChar(State::Operator);
						else
						{
							// If we don't have an operator with the currently appended character then
							// proceed with the current operator

							if (s_Operators.contains(token.value))
								stateNext = State::CompleteToken;
							else
							{
								// If on the current stage we still can't find an operator
								// then probably the operator is not completed yet
								// so continue appending characters

								AppendChar(State::Operator);
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
							throw ParserException("Invalid operator was found: " + token.value);
					}
				}
				break;

				case State::Symbol:
				{
					// Note: we treat all invalid symbols (e.g. 123abc) in the Literal_Numeric state
					if (guard::Symbols[*currentChar] || guard::Digits[*currentChar])
						AppendChar(State::Symbol);
					else
					{
						if (Parser::s_Keywords.contains(token.value))
						{
							// The symbol occurs to be a keyword
							token.type = Token::Type::Keyword;
						}
						else if (token.value == "true" || token.value == "false")
						{
							// The symbol occurs to be boolean
							token.type = Token::Type::Literal_Boolean;
						}

						stateNext = State::CompleteToken;
					}
				}
				break;

				case State::CompleteToken:
				{
					stateNext = State::NewToken;
					tokens.push_back(token);

					token.type = Token::Type::None;
					token.value.clear();
				}
				break;

				}
			}

			stateNow = stateNext;
		}

		if (parenthesesBalancer != 0)
			throw ParserException("Parentheses were not balanced");

		if (quotesBalancer != 0)
			throw ParserException("Quotes were not balanced");

		// Drain out the last token
		if (!token.value.empty())
			tokens.push_back(token);
	}

	std::unordered_map<std::string, Operator> Parser::s_Operators =
	{
		{"=", { Operator::Type::Assign, 0, 2 } },
		{"==", { Operator::Type::Equals, 1, 2 } },
		{"-", { Operator::Type::Subtraction, 2, 2 } },
		{"+", { Operator::Type::Addition, 2, 2 } },
		{"*", { Operator::Type::Multiplication, 3, 2 } },
		{"/", { Operator::Type::Division, 3, 2 } },

		// Unary operators (in that way they are easier to handle)
		{"u-", { Operator::Type::Subtraction, Operator::MAX_PRECEDENCE, 1 } },
		{"u+", { Operator::Type::Addition, Operator::MAX_PRECEDENCE, 1 } },
	};

	std::unordered_map<std::string, Keyword> Parser::s_Keywords =
	{
		{ "if", {} },
		{ "while", {} },
		{ "for", {} }
	};
}
