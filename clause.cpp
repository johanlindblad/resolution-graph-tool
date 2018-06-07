#include "clause.hpp"
#include <map>
#include <algorithm>

Clause::Clause(std::vector<Literal> literals)
{
	this->literals = literals;
	std::sort(this->literals.begin(), this->literals.end(), [](const Literal & a, const Literal & b) -> bool
		{ 
			return a.variable() > b.variable(); 
		}
	);
}

Clause::Clause(const Clause& other)
{
	for(Literal l : other.literals)
	{
		literals.push_back(l);
	}
}

std::string const Clause::to_str() const
{
	std::string out;

	for(const Literal& l : literals)
	{
		out += l.to_str() + " ";
	}

	return out.substr(0, out.length() - 1);
}

Clause Clause::resolve_with(const Clause& other) const
{
	std::map<int, Literal> lits;
	bool removed = false;

	for(Literal l : this->literals)
	{
		lits.insert(std::make_pair(l.variable(), Literal(l)));
	}

	for(Literal l : other.literals)
	{
		if(lits.count(l.variable()) > 0)
		{
			if(lits.at(l.variable()) == l)
			{
				// All OK
			}
			else
			{
				assert(removed == false);
				removed = true;
				lits.erase(l.variable());
			}
		}
		else
		{
			lits.insert(std::make_pair(l.variable(), Literal(l)));
		}
	}

	std::vector<Literal> literals;

	for(std::pair<int, Literal> lit : lits)
	{
		literals.push_back(lit.second);
	}

	return Clause(literals);
}

bool Clause::unit() const
{
	return literals.size() == 1;
}

Literal Clause::first_literal() const
{
	return literals[0];
}

bool Clause::operator==(const Clause& other) const
{
	if(literals.size() != other.literals.size()) return false;

	for(size_t i=0; i < literals.size(); i++)
	{
		if(literals[i] != other.literals[i]) return false;
	}

	return true;
}

std::ostream & operator<<(std::ostream &os, const Clause& c)
{
    return os << c.to_str();
}
