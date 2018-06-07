#include "resolution_graph.hpp"

ResolutionGraph::ResolutionGraph() : decision_level(0)
{
}

void ResolutionGraph::add_clause(std::shared_ptr<Clause> c, int cref)
{
	clauses.push_back(c);
	cref_map.insert(std::make_pair(cref, c));
}

void ResolutionGraph::add_unit(std::shared_ptr<Clause> c)
{
	unit_map.insert(std::make_pair(c->first_literal().variable(), c));
}

void ResolutionGraph::decide(const Literal l)
{
	decision_level += 1;
	index[l.variable()] = trail.size();
	trail.push_back(std::make_tuple(decision_level, l, std::shared_ptr<Clause>()));
}

void ResolutionGraph::propagate(const Literal l, int cref)
{
	assert(cref_map[cref] != nullptr);
	index[l.variable()] = trail.size();
	trail.push_back(std::make_tuple(decision_level, l, cref_map[cref]));
}

void ResolutionGraph::backtrack(int to_level)
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
	std::cout << trail.size() << std::endl;
}

void ResolutionGraph::num_vars(int num_vars)
{
	while(index.size() < num_vars)
	{
		index.push_back(-1);
	}
}

std::shared_ptr<Clause> ResolutionGraph::clause_by_cref(int cref)
{
	assert(cref_map[cref] != nullptr);
	return cref_map[cref];
}
