#pragma once
#include <vector>
#include <algorithm>
#include "literal.hpp"

class Clause
{
public:
	Clause(std::vector<Literal> literals);
	Clause(const Clause& other);
	std::string const to_str() const;
	Clause resolve_with(const Clause& other) const;
	bool unit() const; 
	Literal first_literal() const;
	bool operator ==(const Clause &other) const;
private:
	std::vector<Literal> literals;
	friend std::ostream & operator<<(std::ostream &os, const Clause& c);
};
