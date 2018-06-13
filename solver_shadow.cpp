#include "solver_shadow.hpp"

SolverShadow::SolverShadow(ignore_mode _mode) : mode(_mode), decision_level(0), first_learned_index(-1)
{
}

void SolverShadow::add_clause(std::shared_ptr<const Clause> c, int cref)
{
	int clause_index = clauses.size();
	clauses.push_back(c);
	cref_map.insert(std::make_pair(cref, clause_index));

	if(c->is_learned() && first_learned_index == -1) first_learned_index = clause_index;
}

void SolverShadow::add_unit(std::shared_ptr<const Clause> c)
{
	assert(c->unit());
	add_unit(c, c->first_literal());
}

void SolverShadow::add_unit(std::shared_ptr<const Clause> c, Literal l)
{
	int clause_index = clauses.size();
	clauses.push_back(c);
	unit_map.insert(std::make_pair(l.variable(), clause_index));
}

void SolverShadow::decide(const Literal l)
{
	decision_level += 1;
	index[l.variable()] = trail.size();
	trail.push_back(std::make_tuple(decision_level, l, -1, nullptr));
}

void SolverShadow::propagate(const Literal& l)
{
	assert(unit_map.count(l.variable()) > 0);
	int i = unit_map[l.variable()];
	index[l.variable()] = trail.size();
	trail.push_back(std::make_tuple(decision_level, l, i, clauses[i]));
}

void SolverShadow::propagate(const Literal& l, int cref)
{
	assert(cref_map.count(cref) > 0);
	int clause_index = cref_map[cref];
	assert(clauses[clause_index] != nullptr);
	std::shared_ptr<const Clause> via = clause_by_cref(cref);

	if(decision_level == 0 && mode != none)
	{
		std::vector<clause_ref> chain = {via};
		for(Literal literal : via->literals())
		{
			if(literal != l) chain.push_back(unit_clause(literal));
		}

		clause_index = clauses.size();
		std::shared_ptr<const Clause> new_clause = Clause::resolve(chain);
		add_unit(std::make_shared<Clause>(Clause(*new_clause, true)), l);
	}

	index[l.variable()] = trail.size();
	trail.push_back(std::make_tuple(decision_level, l, clause_index, via));
}

clause_ref SolverShadow::skip(int cref, std::vector<Literal>& literals)
{
	int clause_index = cref_map.at(cref);
	clause_ref clause = clauses[clause_index];
	if(mode == none) return clause;

	std::sort(literals.begin(), literals.end(), [&](const Literal & a, const Literal & b) -> bool
		{ 
			return index[a.variable()] < index[b.variable()]; 
		}
	);

	if(mode == resolve_unit)
	{
		std::vector<clause_ref> units;
		units.push_back(clause);
		for(Literal l : literals)
		{
			assert(unit_map.count(l.variable()) > 0);
			int i = unit_map.at(l.variable());
			clause_ref unit = clauses[i];
			assert(unit != nullptr);
			units.push_back(unit);
		}
		clause_ref result = Clause::resolve(units);
		return result;
	}
	else
	{
		int i = clause_index;
		int first_literal_index = 0;
		std::string key = std::to_string(i) + "_without";

		while(first_literal_index < literals.size())
		{
			Literal l = literals[first_literal_index];
			key += "_" + std::to_string(l.variable());
			first_literal_index += 1;

			if(clauses_with_ignored.count(key) > 0)
			{
				i = clauses_with_ignored.at(key);
			}
			else
			{
				int unit_index = unit_map.at(l.variable());
				clause_ref with_ignored = Clause::resolve(clauses[i], clauses[unit_index]);
				with_ignored = std::make_shared<const Clause>(Clause(*with_ignored, true));
				int new_index = clauses.size();
				clauses.push_back(with_ignored);
				clauses_with_ignored[key] = new_index;
				i = new_index;
			}
		}

		return clauses[i];
	}
}



void SolverShadow::backtrack(int to_level)
{
	while( ! trail.empty())
	{
		trail_item item = trail.back();
		int level = std::get<0>(item);
		Literal l = std::get<1>(item);

		if(level > to_level)
		{
			trail.pop_back();
			index[l.variable()] = -1;
		}
		else
		{
			break;
		}
	}

	decision_level = to_level;
}

void SolverShadow::num_vars(int num_vars)
{
	while(index.size() < num_vars)
	{
		index.push_back(-1);
	}
}

void SolverShadow::restart()
{
	backtrack(0);
}

std::shared_ptr<const Clause> SolverShadow::clause_by_cref(int cref) const
{
	assert(cref_map.count(cref) > 0);
	int clause_index = cref_map.at(cref);
	assert(clauses[clause_index] != nullptr);
	return clauses[clause_index];
}

std::shared_ptr<const Clause> SolverShadow::unit_clause(const Literal& l) const
{
	assert(unit_map.count(l.variable()) > 0);
	int clause_index = unit_map.at(l.variable());
	assert(clauses[clause_index] != nullptr);
	return clauses[clause_index];
}

void SolverShadow::remove_clause(int cref)
{
	int index = cref_map.at(cref);
	cref_map.erase(cref);

	// If we remove the clause from the list, it will not be part of the 
	// "unused graph". However, the memory savings are significant.
	//clauses[index] = std::shared_ptr<const Clause>(nullptr);
}

void SolverShadow::relocate(const std::vector<std::pair<int, int> >& moves)
{
	std::map<int, int> new_mapping(cref_map);

	for(std::pair<int, int> move : moves)
	{
		int from = move.first;
		int to = move.second;

		new_mapping[to] = cref_map.at(from);
		if(new_mapping.at(from) == cref_map.at(from)) new_mapping.erase(from);
	}	

	std::swap(new_mapping, cref_map);
}
	
clause_ref SolverShadow::minimize(clause_ref initial, std::vector<Literal> to_remove) const
{
	std::sort(to_remove.begin(), to_remove.end(), [&](const Literal & a, const Literal & b) -> bool
		{ 
			return index[a.variable()] > index[b.variable()]; 
		}
	);

	clause_ref remaining = initial;

	for(Literal l : to_remove)
	{
		int i = index[l.variable()];
		//std::cout << "Index of " << l << " is " << i << std::endl;
		trail_item item = trail[i];
		clause_ref reason = std::get<3>(item);
		//std::cout << "Reason is " << *reason << std::endl;
		remaining = Clause::resolve(remaining, reason);
	}
	//std::cout << "--" << std::endl;

	return remaining;
}

void SolverShadow::dump_trail() const
{
	for(int i=0; i < trail.size(); i++)
	{
		trail_item item = trail[i];
		std::cout << std::get<0>(item) << ": " << std::get<1>(item);
		if(std::get<2>(item) != -1) std::cout << " via " << *std::get<3>(item);
		std::cout << std::endl;
	}
}
