#include "literal.hpp"
#include <iostream>
#include <string>

Literal::Literal(std::string str)
{
	if(str[0] == '~')
	{
		is_negated = true;
		variable_number = stoi(str.substr(1));
	}
	else
	{
		is_negated = false;
		variable_number = stoi(str);
	}
}

Literal::Literal(const Literal& l)
{
	variable_number = l.variable();
	is_negated = l.negated();
}

std::string const Literal::to_str() const
{
	if(is_negated) return "~" + std::to_string(variable_number);
	return " " + std::to_string(variable_number);
}

int Literal::variable() const
{
	return this->variable_number;
}

bool Literal::negated() const
{
	return this->is_negated;
}

bool Literal::operator==(const Literal& other) const
{
	return variable() == other.variable() && negated() == other.negated();
}

bool Literal::operator!=(const Literal& other) const
{
	return !(operator==(other));
}

std::ostream & operator<<(std::ostream &os, const Literal& l)
{
    return os << l.to_str();
}
