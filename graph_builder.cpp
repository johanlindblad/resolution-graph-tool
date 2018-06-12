#include "graph_builder.hpp"

GraphBuilder::GraphBuilder(const ResolutionGraph& _rg, int conflict_ref, bool _build_graph) : rg(_rg), build_graph(_build_graph)
{
	node_index = 0;

	clause_ref empty_clause = resolve_conflict(conflict_ref);
	assert(empty_clause->empty());
	build_used_graph(empty_clause);
	add_unused();
}

clause_ref GraphBuilder::resolve_conflict(int conflict_ref)
{
	std::shared_ptr<const Clause> remaining = rg.clause_by_cref(conflict_ref);

	// Resolve conflict down to the empty clause
	while( ! remaining->empty())
	{
		// Find literal with max index
		Literal last = remaining->literals().front();
		for(Literal& l : remaining->literals())
		{
			if(rg.index[l.variable()] > rg.index[last.variable()]) last = l;
		}

		int i = rg.index[last.variable()];
		trail_item t = rg.trail[i];
		int reason_index = std::get<2>(t);
		clause_ref reason = std::get<3>(t);
		remaining = Clause::resolve(remaining, reason);
	}

	return remaining;
}

void GraphBuilder::build_used_graph(clause_ref empty_clause)
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

void GraphBuilder::add_unused()
{
	typedef std::pair<clause_ref, int> queue_item;
	std::queue<queue_item> queue;

	// Next, add all unvisited learned clauses to the queue and traverse
	// the whole unused part of the graph
	if(rg.first_learned_index != -1)
	{
		for(int i=rg.first_learned_index; i < rg.clauses.size(); i++)
		{
			clause_ref c = rg.clauses[i];
			if(c == nullptr) continue;

			bool unexplained = learned_clause_index.count(c.get()) == 0;
			if(unexplained) queue.push(queue_item(c, next_index()));
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

void GraphBuilder::print_graphviz() const
{
	assert(build_graph);
	label_writer wr(g);
	boost::write_graphviz(std::cout, g, wr);
}

void GraphBuilder::clear_unused()
{
	
}

statistics GraphBuilder::vertex_statistics() const
{
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

int GraphBuilder::next_index()
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
