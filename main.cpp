#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include "literal.hpp"
#include "clause.hpp"
#include "resolution_graph.hpp"
#include "graph_builder.hpp"

int main()
{
	ignore_mode ignore_mode = learn;

	std::string line;
	ResolutionGraph r(ignore_mode);
	while(std::getline(std::cin, line))
	{
		std::istringstream ss(line);

		std::string instruction;
		ss >> instruction;

		//std::cout << line << std::endl;

		if(instruction == "NV")
		{
			int num;
			ss >> num;
			
			r.num_vars(num);
		}
		else if(instruction == "I")
		{
			int ref, num_literals;
			std::vector<Literal> literals;

			ss >> ref >> num_literals;

			std::string ls;
			for(int i=0; i < num_literals; i++)
			{
				ss >> ls;
				literals.push_back(Literal(ls));
			}

			r.add_clause(std::make_shared<Clause>(Clause(literals)), ref);
		}
		else if(instruction == "D")
		{
			std::string ls;
			ss >> ls;
			r.decide(ls);
		}
		else if(instruction == "P")
		{
			std::string ls;
			int ref;
			ss >> ls >> ref;
			r.propagate(ls, ref);
		}
		else if(instruction == "PU")
		{
			std::string ls;
			ss >> ls;
			Literal l(ls);
			r.propagate(l);
		}
		else if(instruction == "U")
		{
			std::vector<Literal> empty = {};
			std::shared_ptr<const Clause> remaining;

			while(true)
			{
				bool should_read = true;

				//std::cout << line << std::endl;

				if(instruction == "U")
				{
					int ref;
					ss >> ref;
					std::vector<Literal> to_skip;

					while(true)
					{
						// Read one more line and see if we get an "S" instruction
						std::getline(std::cin, line);
						ss = std::istringstream(line);
						ss >> instruction;
						should_read = false;
					
						if(instruction == "S")
						{
							int num_skipped;
							ss >> num_skipped;
							should_read = true;

							std::string literal;

							for(int i=0; i < num_skipped; i++)
							{
								ss >> literal;
								to_skip.push_back(Literal(literal));
							}

						}
						else
						{
							break;
						}
					}

					std::shared_ptr<const Clause> c = r.clause_by_cref(ref);
					//std::cout << "Using " << ref << " which is " << *c << std::endl;


					if(to_skip.size() > 0)
					{
						c = r.skip(ref, to_skip);
						//std::cout << "After skip " << *c << std::endl;
					}

					if(remaining == nullptr)
					{
						remaining = c;
					}
					else
					{
						remaining = Clause::resolve(remaining, c);
					}

				}
				else if(instruction == "LU")
				{
					std::string l;
					ss >> l;
					Literal expected_unit(l);

					assert(remaining->unit() || ignore_mode == none);
					assert(remaining->first_literal() == expected_unit || ignore_mode == none);
					r.add_unit(std::make_shared<const Clause>(Clause(*remaining, true)), expected_unit);
					break;
				}
				else if(instruction == "L" || instruction == "LU")
				{
					
					int ref, num_literals;
					std::vector<Literal> literals;
					ss >> ref >> num_literals;

					std::string ls;
					for(int i=0; i < num_literals; i++)
					{
						ss >> ls;
						literals.push_back(Literal(ls));
					}

					Clause should_be(literals);

					//std::cout << line << std::endl;
					//std::cout << should_be << " vs " << *remaining << std::endl;
					//r.dump_trail();
					assert(should_be == *remaining.get() || ignore_mode == none);

					r.add_clause(std::make_shared<const Clause>(Clause(*remaining, true)), ref);
					break;
				}
				else if(instruction == "MNM")
				{
					int count;
					ss >> count;
					std::vector<Literal> removed_literals;
					std::string ls;

					for(int i=0; i < count; i++)
					{
						ss >> ls;
						removed_literals.push_back(Literal(ls));
					}

					remaining = r.minimize(remaining, removed_literals);
				}
				else if(instruction == "B")
				{
					int level;
					ss >> level;
					r.backtrack(level);
				}
				else
				{
					std::cout << instruction << std::endl;
					assert(instruction == "");
				}

				// Some branches need to read input
				// If they have been executed, do not read one more line
				if( ! should_read) continue;

				if( ! std::getline(std::cin, line)) break;
				ss = std::istringstream(line);
				ss >> instruction;
			}
		}
		else if(instruction == "B")
		{
			int level;
			ss >> level;

			r.backtrack(level);

		}
		else if(instruction == "RS")
		{
			r.restart();
		}
		else if(instruction == "C")
		{
			int ref;
			ss >> ref;

			GraphBuilder gb(r, ref, true);
			//gb.print_graphviz();
			statistics s = gb.vertex_statistics();

			std::cout << "Axioms: " << s.used_axioms << " used vs " << s.unused_axioms << " unused" << std::endl;
			std::cout << "Learned: " << s.used_learned << " used vs " << s.unused_learned << " unused" << std::endl;
			std::cout << "Intermediate: " << s.used_intermediate << " used vs " << s.unused_intermediate << " unused" << std::endl;
			std::cout << "Tree violations " << s.tree_edge_violations << " edges to " << s.tree_vertex_violations << " vertices" << std::endl;
			std::cout << "Regularity violations " << s.regularity_edge_violations << " edges on " << s.regularity_path_violations << " paths" << std::endl;
		}
		else if(instruction == "R")
		{
			int ref;
			ss >> ref;
			r.remove_clause(ref);
		}
		else if(instruction == "M")
		{
			std::vector<std::pair<int, int> > moves;

			while(true)
			{
				if(instruction == "M")
				{
					int from, to;
					ss >> from >> to;
					moves.push_back(std::make_pair(from, to));
				}
				else if(instruction == "RD")
				{
					r.relocate(moves);
					break;
				}

				if( ! std::getline(std::cin, line)) break;
				ss = std::istringstream(line);
				ss >> instruction;
			}
		}
		else if(instruction == "RD")
		{
		}
		else
		{
			//std::cout << instruction << std::endl;
		}
	}
}
