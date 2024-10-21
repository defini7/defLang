#pragma once

#include <limits>

namespace def
{
	struct Operator
	{
		enum class Type
		{
			Subtraction,
			Addition,
			Multiplication,
			Division,
			Equals,
			Assign
		};

		Type type;
		unsigned char precedence;
		unsigned char arguments;

		static constexpr unsigned char MAX_PRECEDENCE = std::numeric_limits<unsigned char>::max();
	};
}
