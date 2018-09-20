#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include "solver_shadow.hpp"

// State that is needed by vertex (i.e. by clause)
struct vertex_info
{ 
    clause_ref clause;
    bool used = true;
};

// The statistics that the grapher keeps track of
struct statistics
{
	long long used_axioms = 0, used_intermediate = 0, used_learned = 0,
	    unused_axioms = 0, unused_intermediate = 0, unused_learned = 0,
	    tree_edge_violations = 0, tree_vertex_violations = 0,
	    regularity_violations_total = 0, regularity_violation_variables = 0,
	    width = 0, copy_cost = 0;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, vertex_info> Graph;

class label_writer
{
	public:
		label_writer(const Graph& g) : graph(g) {}
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
		const Graph& graph;
};

class edge_label_writer
{
	public:
		edge_label_writer(const Graph& g) : graph(g) {}
		template <class VertexOrEdge>
			void operator()(std::ostream& out, const VertexOrEdge& e) const
			{
				out << "[label=\"" << graph[source(e, graph)].clause->removed_variable().value() << "\"]";
			}
	private:
		const Graph& graph;
};
