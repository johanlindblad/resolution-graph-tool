#pragma once
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include <assert.h>
#include "clause.hpp"
#include "literal.hpp"

enum ignore_mode { none, learn, resolve_unit };

typedef std::shared_ptr<const Clause> clause_ref;
// decision level, assignment, reason clause index, reason clause pointer
// (pointer only used to allow removing clauses from database without making
// reference invalid)
typedef std::tuple<int, const Literal, int, clause_ref> trail_item;

// Solver shadow represents the state of the solver as it appears from the trace
// Because the graph needs to be reconstructed afterwards, it also contains
// some information explicitly that minisat keeps implicitly
// (for example, learned clauses contain references to their resolution chains)
class SolverShadow
{
public:
	SolverShadow(ignore_mode _mode);
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
	clause_ref minimize(clause_ref initial, std::vector<Literal> to_remove) const;
	// Full is the mode that allows temporary new literals (intermediate steps in the
	// implication graph)
	clause_ref minimize_full(clause_ref initial, std::vector<Literal> to_remove) const;

	std::shared_ptr<const Clause> clause_by_cref(int cref) const;
	std::shared_ptr<const Clause> unit_clause(const Literal& l) const;

	void dump_trail() const;

	friend class ResolutionGraph;
private:
	int num_vars() const;

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
