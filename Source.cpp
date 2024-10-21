#include <iostream>

#include "Interpreter.hpp"

int main()
{
	def::Parser parser;
	def::Interpreter interpreter;

	std::string input;

	// Simple REPL
	do
	{
		std::cout << "> ";
		std::getline(std::cin, input);

		try
		{
			std::vector<def::Token> tokens;
			parser.Tokenise(input, tokens);

			for (const auto& token : tokens)
				std::cout << token.ToString() << std::endl;

			auto result = interpreter.Solve(tokens);

			if (result)
			{
				std::visit([](const auto& arg)
					{
						if (std::is_same_v<decltype(arg), const def::Boolean&>)
							std::cout << std::boolalpha;

						std::cout << arg.value << std::noboolalpha << std::endl;
					}, result.value());
			}
		}
		catch (const def::ParserException& e)
		{
			std::cerr << e.what() << std::endl;
		}
	} while (input != "quit");

	return 0;
}
