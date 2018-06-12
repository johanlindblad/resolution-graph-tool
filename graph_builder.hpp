#pragma once
#include "resolution_graph.hpp"
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

struct vertex_info
{ 
    clause_ref clause;
    bool used = true;
};

struct statistics
{
	int used_axioms = 0, used_intermediate = 0, used_learned = 0,
	    unused_axioms = 0, unused_intermediate = 0, unused_learned = 0,
	    tree_edge_violations = 0, tree_vertex_violations = 0,
	    regularity_edge_violations = 0, regularity_path_violations = 0;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, vertex_info> graph;

class label_writer
{
	public:
		label_writer(const graph& g) : graph(g) {}
		template <class VertexOrEdge>
			void operator()(std::ostream& out, const VertexOrEdge& v) const
			{
				assert(graph[v].clause != nullptr);
				clause_ref clause = graph[v].clause;

				out << "[label=\"" << *clause << "\"]";
				if(clause->is_axiom()) out << " [style=filled]";
				else if(clause->is_learned()) out << " [style=filled] [fillcolor=turquoise1]";

				if( ! graph[v].used) out << " [fontsize=6] [width=0.25] [height=0.25]";
			}
	private:
		const graph& graph;
};

class edge_label_writer
{
	public:
		edge_label_writer(const graph& g) : graph(g) {}
		template <class VertexOrEdge>
			void operator()(std::ostream& out, const VertexOrEdge& e) const
			{
				out << "[label=\"" << graph[source(e, graph)].clause->removed_variable().value() << "\"]";
			}
	private:
		const graph& graph;
};

class GraphBuilder
{
public:
	GraphBuilder(const ResolutionGraph& _rg, int conflict_ref, bool build_graph);
	void print_graphviz() const;
	void clear_unused();
	statistics vertex_statistics() const;
	void calculate_regularity_measures();

private:
	clause_ref resolve_conflict(int conflict_ref);
	void build_used_graph();
	void add_unused();
	int next_index();

	const ResolutionGraph& rg;
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
