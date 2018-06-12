#pragma once
#include <vector>
#include <memory>
#include <map>
#include <queue>
#include <assert.h>
#include "clause.hpp"
#include "literal.hpp"

enum ignore_mode { none, learn, resolve_unit };

typedef std::shared_ptr<const Clause> clause_ref;
// decision level, assignment, reason clause index
typedef std::tuple<int, const Literal, int> trail_item;


class ResolutionGraph
{
public:
	ResolutionGraph(ignore_mode _mode);
	void add_clause(const clause_ref c, int cref);
	void add_unit(const clause_ref c);

	// Needed because without literal skipping, we might learn a wider clause
	// that should still be treated as a unit
	void add_unit(const clause_ref c, const Literal l);

	void decide(const Literal l);
	void propagate(const Literal& l, int cref);
	void propagate(const Literal& l);
	void backtrack(int to_level);
	void num_vars(int num_vars);
	void restart();
	void remove_clause(int cref);
	void relocate(const std::vector<std::pair<int, int> >& moves);
	clause_ref skip(int cref, std::vector<Literal>& skipped);

	std::shared_ptr<const Clause> clause_by_cref(int cref) const;
	std::shared_ptr<const Clause> unit_clause(const Literal& l) const;

	friend class GraphBuilder;
private:
	std::vector<clause_ref> clauses;
	std::map<int, int> cref_map;
	std::map<int, int> unit_map;
	std::vector<int> index;

	int decision_level;
	std::vector<trail_item> trail;
	int first_learned_index;
	ignore_mode mode;
	std::map<std::string, int> clauses_with_ignored;
};
