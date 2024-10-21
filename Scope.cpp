#include "Scope.hpp"

namespace def
{
	Scope::Scope(Scope* parent) : m_Parent(parent)
	{
	}

	void Scope::Assign(const std::string& name, const Object& value)
	{
		if (!m_Values.contains(name))
		{
			// It can't find a variable in the current scope

			if (m_Parent)
			{
				// If we have the parent scope let's try to find the variable in it
				return m_Parent->Assign(name, value);
			}

			// It occurs that we are in the global scope and we can't find variable here,
			// that means variable doesn't exist so let's create a new one in the current scope
		}

		// We found a variable in the current scope or it doesn't exist
		// so let's assign a value to it
		m_Values[name] = value;
	}

	std::optional<std::reference_wrapper<Object>> Scope::Get(const std::string& name)
	{
		if (!m_Values.contains(name))
		{
			// We can't find a variable in the current scope

			if (m_Parent)
			{
				// Let's try to find it in the parent scopes
				return m_Parent->Get(name);
			}

			// No parent scope so the variable doesn't exist
			return std::nullopt;
		}

		return m_Values[name];
	}
}
