#include "graph_builder.hpp"

GraphBuilder::GraphBuilder(const ResolutionGraph& _rg, int conflict_ref, bool _build_graph) : rg(_rg), build_graph(_build_graph)
{
	node_index = 0;

	empty_clause = resolve_conflict(conflict_ref);
	assert(empty_clause->empty());
	build_used_graph();
	add_unused();

	// This takes a very long time
	calculate_regularity_measures();
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

void GraphBuilder::build_used_graph()
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
	edge_label_writer ewr(g);
	boost::write_graphviz(std::cout, g, wr, ewr);
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

void GraphBuilder::calculate_regularity_measures()
{
	std::map<int, int> times_used;

	enum vertex_status { unhandled, used_first, used_both };
	typedef std::pair<clause_ref, vertex_status> state;
	std::stack<state> stack;
	stack.push(state(empty_clause, unhandled));
	// Our stack consists of complete "breadcrumbs", with full path
	// as well as the status of all nodes
	// We first go "left" (first parent) as far as we can,
	// then "right" (second parent) as far as we can and finally
	// when both paths are explored, go back to parent and mark
	// the resolved away literal as used one time less

	while( ! stack.empty())
	{
		state s = stack.top();stack.pop();

		clause_ref go_from = s.first;
		vertex_status status = s.second;

		//std::cout << "At " << *go_from << " with status " << status << std::endl;

		if(go_from->is_axiom())
		{
			//std::cout << "Reached leaf " << *go_from << std::endl;
			bool violations = false;

			for(std::pair<int, int> p : times_used)
			{
				if(p.second > 1)
				{
					this->s.regularity_edge_violations += p.second - 1;
					violations = true;
				}
			}

			if(violations) this->s.regularity_path_violations++;

		}
		else if(status == unhandled)
		{
			int removed_variable = go_from->removed_variable().value();
			times_used[removed_variable]++;
			stack.push(state(go_from, used_first));
			stack.push(state(go_from->resolved_from().first, unhandled));
		}
		else if(status == used_first)
		{
			stack.push(state(go_from, used_both));
			stack.push(state(go_from->resolved_from().second, unhandled));
		}
		else if(status == used_both)
		{
			int removed_variable = go_from->removed_variable().value();
			times_used[removed_variable]--;
			//std::cout << "Done with " << *go_from << std::endl;
		}
	}

	// When done, all variables should be marked as used 0 times
	// Otherwise, we forgot to subtract somewhere
	for(auto it=times_used.begin(); it != times_used.end(); it++)
	{
		int times = it->second;
		assert(times == 0);
	}
}
