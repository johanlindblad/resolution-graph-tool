#pragma once
#include "solver_shadow.hpp"
#include "resolution_graph_extras.hpp"

// ResolutionGraph takes the information from the solver shadow and
// calculates statistics on the resolution graph (and, given the build_graph
// parameter, builds a graph using the Boost library, which can be printed as
// graphviz)
class ResolutionGraph
{
public:
	ResolutionGraph(const SolverShadow& _rg, int conflict_ref, bool build_graph);
	void print_graphviz() const;
	statistics vertex_statistics() const;
	void remove_unused();
private:
	clause_ref resolve_conflict(int conflict_ref);
	void build_used_graph();
	void add_unused();
	int next_index();

	const SolverShadow& rg;
	graph g;
	std::map<const Clause*, int> learned_clause_index;
	statistics s;
	// Keep track of all learned clauses that have been used more than once
	std::set<const Clause*> violating_learned;
	const bool build_graph;
	clause_ref empty_clause;

	// Used when graph is not built
	int node_index;
};
