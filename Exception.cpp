#include "Exception.hpp"

namespace def
{
	Exception::Exception(const std::string& message)
	{
		m_Message = message;
	}

	const char* Exception::what() const
	{
		return m_Message.c_str();
	}

	ParserException::ParserException(const std::string& message) : Exception("[Parse Error] " + message)
	{

	}

	InterpreterException::InterpreterException(const std::string& message) : Exception("[Interpret Error] " + message)
	{

	}
}
