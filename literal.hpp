#pragma once
#include <string>
#include <iostream>

class Literal
{
public:
	Literal(std::string str);
	Literal(const Literal& l);
	std::string const to_str() const;
	int variable() const;
	bool negated() const;
	bool operator ==(const Literal &other) const;
	bool operator !=(const Literal &other) const;

private:
	int variable_number;
	bool is_negated;
	friend std::ostream & operator<<(std::ostream &os, const Literal& l);
};
