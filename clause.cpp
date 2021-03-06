#include "clause.hpp"
#include <map>
#include <algorithm>

Clause::Clause(std::vector<Literal> literals)
{
	this->literal_vector = literals;
	std::sort(this->literal_vector.begin(), this->literal_vector.end(), [](const Literal & a, const Literal & b) -> bool
		{ 
			return a.variable() < b.variable(); 
		}
	);

	this->learned = false;
	this->removed_var = boost::none;

	this->_violated_regularity = false;

	// An axiom clause has no parents, and so copying it costs 1
	this->cost = 1;
}

Clause::Clause(std::vector<Literal> literals, const std::pair<std::shared_ptr<const Clause>, std::shared_ptr<const Clause> > source, int removed) : Clause(literals)
{
	this->parents = source;
	this->learned = false;
	this->removed_var = removed;
	this->_violated_regularity = false;

	this->removed_variables = source.first->removed_variables;
	const std::vector<bool>& other_removed = source.second->removed_variables;

	size_t required_size = std::max(other_removed.size(), (size_t) removed + 1);
	if(this->removed_variables.size() < required_size) this->removed_variables.resize(required_size);

	for(int i=0; i < other_removed.size(); i++) if(other_removed[i] == true) this->removed_variables[i] = true;

	// Find out if the current clause was the direct result of a violation
	if(this->removed_variables[removed] == true)
	{
		this->_violated_regularity = true;
		this->_violated_regularity_variable = removed;
	}
	this->removed_variables[removed] = true;

	// An intermediate clause costs 1 to copy itself, plus whatever the parents cost
	this->cost = 1 + source.first->cost + source.second->cost;
}

Clause::Clause(const Clause& other)
{
	literal_vector = other.literals();

	this->parents = other.parents;
	this->learned = other.learned;
	this->removed_var = other.removed_var;
	this->removed_variables = other.removed_variables;
	this->cost = other.cost;
	this->_violated_regularity = other._violated_regularity;
	this->_violated_regularity_variable = other._violated_regularity_variable;
}

Clause::Clause(const Clause& other, bool is_learned) : Clause(other)
{
	this->learned = is_learned;
	assert(this->is_resolvent());
}

std::string const Clause::to_str() const
{
	std::string out;

	for(const Literal& l : literal_vector)
	{
		out += l.to_str() + " ";
	}

	return out.substr(0, out.length() - 1);
}

std::shared_ptr<const Clause> Clause::resolve(const std::shared_ptr<const Clause>& clause, const std::shared_ptr<const Clause>& other)
{
	const std::vector<Literal>& lits1 = clause->literals();
	const std::vector<Literal>& lits2 = other->literals();
	auto it1 = lits1.begin();
	auto it2 = lits2.begin();
	std::vector<Literal> out;
	boost::optional<int> removed;

	while(it1 != lits1.end() && it2 != lits2.end())
	{
		if(it1->variable() < it2->variable())
		{
			out.push_back(*it1);
			it1++;
		}
		else if(it1->variable() > it2->variable())
		{
			out.push_back(*it2);
			it2++;
		}
		else
		{
			if(*it1 == *it2)
			{
				out.push_back(*it1);
			}
			else
			{
				assert(removed == boost::none);
				removed = it1->variable();
			}

			it1++;
			it2++;
		}
	}

	while(it1 != lits1.end())
	{
		out.push_back(*it1);
		it1++;
	}
	while(it2 != lits2.end())
	{
		out.push_back(*it2);
		it2++;
	}

	assert(removed != boost::none);
	
	return std::make_shared<Clause>(Clause(out, std::make_pair(clause, other), removed.value()));
}

std::shared_ptr<const Clause> Clause::resolve(const std::vector<std::shared_ptr<const Clause> > clauses)
{
	auto iterator = clauses.begin();
	std::shared_ptr<const Clause> remaining = *iterator;
	iterator++;

	for(; iterator != clauses.end(); iterator++) remaining = resolve(remaining, *iterator);

	return remaining;
}

bool Clause::unit() const
{
	return literal_vector.size() == 1;
}

Literal Clause::first_literal() const
{
	return literal_vector[0];
}

bool Clause::operator==(const Clause& other) const
{
	if(literal_vector.size() != other.literals().size()) return false;

	for(size_t i=0; i < literal_vector.size(); i++)
	{
		if(literal_vector[i] != other.literals()[i]) return false;
	}

	return true;
}

std::vector<Literal> Clause::literals() const
{
	return std::vector<Literal>(this->literal_vector);
}

int Clause::width() const
{
	return this->literal_vector.size();
}

const std::pair<std::shared_ptr<const Clause>, std::shared_ptr<const Clause> > Clause::resolved_from() const
{
	return this->parents;
}

bool Clause::is_resolvent() const
{
	return this->parents.first != nullptr;
}

bool Clause::empty() const
{
	return this->literals().size() == 0;
}

bool Clause::is_learned() const
{
	return this->learned;
}

void Clause::is_learned(bool l)
{
	this->learned = l;
}

bool Clause::is_axiom() const
{
	return ! this->is_resolvent();
}

std::ostream & operator<<(std::ostream &os, const Clause& c)
{
    return os << c.to_str();
}

boost::optional<int> Clause::removed_variable() const
{
	return this->removed_var;
}

long double Clause::copy_cost() const
{
	return this->cost;
}

bool Clause::violated_regularity() const
{
	return this->_violated_regularity;
}

long Clause::violated_regularity_variable() const
{
	return this->_violated_regularity_variable;
}
