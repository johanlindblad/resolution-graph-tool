#include "resolution_graph.hpp"

ResolutionGraph::ResolutionGraph(const SolverShadow& _solver, int conflict_ref, bool _build_graph) : solver(_solver), build_graph(_build_graph)
{
	node_index = 0;

	empty_clause = resolve_conflict(conflict_ref);
	assert(empty_clause->empty());
	s.copy_cost = empty_clause->copy_cost();
	build_used_graph();
	add_unused();

	// This will give the result for the resolution refutation, not including any
	// violations for unused learned clauses
	// (those could, if relevant, be counted too)
	s.regularity_violations_total = empty_clause->num_regularity_violations();
	s.regularity_violation_variables = empty_clause->regularity_violation_variables().size();
}

// Start with the final conflict clause and resolve with the reasons for all variables,
// in reverse assignment order
clause_ref ResolutionGraph::resolve_conflict(int conflict_ref)
{
	std::shared_ptr<const Clause> remaining = solver.clause_by_cref(conflict_ref);

	// Resolve conflict down to the empty clause
	while( ! remaining->empty())
	{
		// Find literal with max index
		Literal last = remaining->literals().front();
		for(Literal& l : remaining->literals())
		{
			if(solver.index[l.variable()] > solver.index[last.variable()]) last = l;
		}

		int i = solver.index[last.variable()];
		trail_item t = solver.trail[i];
		int reason_index = std::get<2>(t);
		clause_ref reason = std::get<3>(t);
		remaining = Clause::resolve(remaining, reason);
	}

	return remaining;
}

void ResolutionGraph::build_used_graph()
{
	// Start from the empty clause and do a BFS to build complete graph
	// of all used nodes
	typedef std::pair<clause_ref, int> queue_item;
	std::queue<queue_item> queue;
	queue.push(queue_item(empty_clause, next_index()));

	while( ! queue.empty())
	{
		queue_item item = queue.front();queue.pop();
		clause_ref clause = item.first;
		int index = item.second;
		if(build_graph) g[index].clause = clause;

		if(clause->is_axiom()) s.used_axioms++;
		else if(clause->is_learned()) s.used_learned++;
		else s.used_intermediate++;

		s.width = std::max(s.width, (long long) clause->width());

		// Add all unvisited children to the graph
		// and queue up all learned clauses for further
		// traversal
		if(clause->is_resolvent())
		{
			std::pair<clause_ref, clause_ref> parents = clause->resolved_from();

			int sub_index_1, sub_index_2;
			if(parents.first->is_learned() == false || learned_clause_index.count(parents.first.get()) == 0)
			{
				sub_index_1 = next_index();
				queue.push(queue_item(parents.first, sub_index_1));
				if(parents.first->is_learned()) learned_clause_index[parents.first.get()] = sub_index_1;
			}
			else
			{
				sub_index_1 = learned_clause_index.at(parents.first.get());
				s.tree_edge_violations++;
				violating_learned.insert(parents.first.get());
			}
			if(build_graph) boost::add_edge(index, sub_index_1, g);

			if(parents.second->is_learned() == false || learned_clause_index.count(parents.second.get()) == 0)
			{
				sub_index_2 = next_index();
				queue.push(queue_item(parents.second, sub_index_2));
				if(parents.second->is_learned()) learned_clause_index[parents.second.get()] = sub_index_2;
			}
			else
			{
				sub_index_2 = learned_clause_index.at(parents.second.get());
				s.tree_edge_violations++;
				violating_learned.insert(parents.second.get());
			}
			if(build_graph) boost::add_edge(index, sub_index_2, g);
		}
	}

	s.tree_vertex_violations = violating_learned.size();
}

void ResolutionGraph::add_unused()
{
	typedef std::pair<clause_ref, int> queue_item;
	std::queue<queue_item> queue;

	// Next, add all unvisited learned clauses to the queue and traverse
	// the whole unused part of the graph
	if(solver.first_learned_index != -1)
	{
		for(int i=solver.first_learned_index; i < solver.clauses.size(); i++)
		{
			clause_ref c = solver.clauses[i];
			if(c == nullptr) continue;

			assert(c->is_learned());
			bool unexplained = learned_clause_index.count(c.get()) == 0;

			if(unexplained)
			{
				int index = next_index();
				queue.push(queue_item(c, index));
				learned_clause_index[c.get()] = index;
			}

		}

		while( ! queue.empty())
		{
			queue_item item = queue.front();queue.pop();
			clause_ref clause = item.first;
			int index = item.second;

			if(build_graph)
			{
				g[index].clause = clause;
				g[index].used = false;
			}

			if(clause->is_axiom()) s.unused_axioms++;
			else if(clause->is_learned()) s.unused_learned++;
			else s.unused_intermediate++;

			if( ! clause->is_resolvent()) continue;

			int sub_index_1, sub_index_2;
			std::pair<clause_ref, clause_ref> parents = clause->resolved_from();

			bool already_used = parents.first->is_learned() && learned_clause_index.count(parents.first.get()) > 0;

			if(already_used)
			{
				sub_index_1 = learned_clause_index[parents.first.get()];
			}
			else
			{
				sub_index_1 = next_index();
				queue.push(queue_item(parents.first, sub_index_1));
				if(parents.first->is_learned()) learned_clause_index[parents.first.get()] = sub_index_1;
			}
			
			if(build_graph) boost::add_edge(index, sub_index_1, g);

			already_used = parents.second->is_learned() && learned_clause_index.count(parents.second.get()) > 0;

			if(already_used)
			{
				sub_index_2 = learned_clause_index[parents.second.get()];
			}
			else
			{
				sub_index_2 = next_index();
				queue.push(queue_item(parents.second, sub_index_2));
				if(parents.second->is_learned()) learned_clause_index[parents.second.get()] = sub_index_2;
			}
			
			if(build_graph) boost::add_edge(index, sub_index_2, g);
		}
	}
}

void ResolutionGraph::print_graphviz(std::ostream& stream) const
{
	assert(build_graph);
	label_writer wr(g);
	edge_label_writer ewr(g);
	boost::write_graphviz(stream, g, wr, ewr);
}

statistics ResolutionGraph::vertex_statistics() const
{
	// Commented-out part which recalculates (some of) the statistics
	/*statistics s;

	TODO: tree violations
	for(auto it=vertices(g).first; it != vertices(g).second; it++)
	{
		clause_ref c = g[*it].clause;

		if(g[*it].used)
		{
			if(c->is_axiom()) s.used_axioms++;
			else if(c->is_learned()) s.used_learned++;
			else s.used_intermediate++;
		}
		else
		{
			if(c->is_axiom()) s.unused_axioms++;
			else if(c->is_learned()) s.unused_learned++;
			else s.unused_intermediate++;
		}
	}*/


	return s;
}

int ResolutionGraph::next_index()
{
	if(build_graph)
	{
		return boost::add_vertex(g);
	}
	else
	{
		node_index++;
		return node_index - 1;
	}
}

void ResolutionGraph::remove_unused()
{
	assert(build_graph);

	for(auto it=vertices(g).first; it != vertices(g).second; it++)
	{
		if(! g[*it].used)
		{
			boost::clear_vertex(*it, g);
			boost::remove_vertex(*it, g);
			it--;
		}
	}
}
