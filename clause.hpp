#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/optional.hpp>
#include "literal.hpp"

#include <memory>

class Clause
{
public:
	Clause(std::vector<Literal> literals);
	Clause(std::vector<Literal> literals, const std::pair<std::shared_ptr<const Clause>, std::shared_ptr<const Clause> > source, int removed);
	Clause(const Clause& other);

	// Separate copy constructor for modifying is_learned (allows for const everywhere else)
	Clause(const Clause& other, bool is_learned);
	std::string const to_str() const;
	bool unit() const; 
	Literal first_literal() const;
	bool operator ==(const Clause &other) const;
	std::vector<Literal> literals() const;
	int width() const;
	const std::pair<std::shared_ptr<const Clause>, std::shared_ptr<const Clause> > resolved_from() const;
	bool is_resolvent() const;
	bool empty() const;
	bool is_learned() const;
	void is_learned(bool l);
	bool is_axiom() const;
	boost::optional<int> removed_variable() const;
	long long num_regularity_violations() const;
	std::vector<int> regularity_violation_variables() const;
	long double copy_cost() const;

	static std::shared_ptr<const Clause> resolve(const std::shared_ptr<const Clause>& clause, const std::shared_ptr<const Clause>& other);
	static std::shared_ptr<const Clause> resolve(const std::vector<std::shared_ptr<const Clause> > clauses);
private:
	std::vector<Literal> literal_vector;
	friend std::ostream & operator<<(std::ostream &os, const Clause& c);
	std::pair<std::shared_ptr<const Clause>, std::shared_ptr<const Clause> > parents;
	bool learned;
	boost::optional<int> removed_var;
	std::vector<bool> removed_variables;
	std::vector<bool> reremoved_variables;
	long long regularity_violations;
	long double cost;
};
