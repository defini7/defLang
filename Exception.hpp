#pragma once

#include <string>

namespace def
{
	struct Exception : std::exception
	{
		Exception(const std::string& message);

		const char* what() const override;

	private:
		std::string m_Message;

	};

	struct ParserException : Exception
	{
		ParserException(const std::string& message);
	};

	struct InterpreterException : Exception
	{
		InterpreterException(const std::string& message);
	};
}
