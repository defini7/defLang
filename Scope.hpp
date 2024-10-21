#pragma once

#include <unordered_map>
#include <string>
#include <variant>
#include <optional>

namespace def
{
	template <class T>
	struct Type
	{
		T value;
	};

	struct Numeric : Type<long double> {};
	struct Boolean : Type<bool> {};
	struct String : Type<std::string> {};
	struct Symbol : Type<std::string> {};

	using Object = std::variant<Numeric, Boolean, String, Symbol>;

	class Scope
	{
	public:
		Scope(Scope* parent = nullptr);

	public:
		void Assign(const std::string& name, const Object& value);

		std::optional<std::reference_wrapper<Object>> Get(const std::string& name);

	private:
		Scope* m_Parent;

		std::unordered_map<std::string, Object> m_Values;

	};
}
