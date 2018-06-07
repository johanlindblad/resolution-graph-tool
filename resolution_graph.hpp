#pragma once
#include <vector>
#include <memory>
#include <map>
#include <assert.h>
#include "clause.hpp"
#include "literal.hpp"

typedef std::shared_ptr<Clause> clause_ref;
// decision level, assignment, reason clause
typedef std::tuple<int, const Literal, const clause_ref> trail_item;

class ResolutionGraph
{
public:
	ResolutionGraph();
	void add_clause(const clause_ref c, int cref);
	void add_unit(const clause_ref c);
	void decide(const Literal l);
	void propagate(const Literal l, int cref);
	void backtrack(int to_level);
	void num_vars(int num_vars);

	std::shared_ptr<Clause> clause_by_cref(int cref);
private:
	std::vector<clause_ref> clauses;
	std::map<int, clause_ref> cref_map;
	std::map<int, clause_ref> unit_map;
	std::vector<int> index;

	int decision_level;
	std::vector<trail_item> trail;
};
