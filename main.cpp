#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include "literal.hpp"
#include "clause.hpp"
#include "solver_shadow.hpp"
#include "resolution_graph.hpp"
#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
	// The three ignore modes modes are:
	// 0. none => literals are not skipped (they are guaranteed to be removed during conflict resolution)
	//            makes for the most tree-like resolution
	// 1. learn => learn smaller clauses from resolving learned clauses or axioms with learned units
	//             can make learned clause derivation non-trivial
	// 2. resolve_unit => resolve with learned units to remove skipped literals, immediately after
	// 		      using learned clause/axiom. Keeps learned clause derivation trivial.
	//
	// (modes 2 and 3 introduce regularity violations because skipped literals will also be
	// resolved away during final conflict resolution)
	ignore_mode mode;

	bool print_graph = false;
	bool print_with_unused = false;

	boost::program_options::options_description desc("Supported options");
	desc.add_options()
		("help", "show this help")
		("ignore-mode", boost::program_options::value<int>(), "ignore mode (0=none, 1=learn, 2=resolve_unit) (see code for details)")
		("print-graph", "print out resolution graph as DOT (instead of statistics)")
		("include-unused", "include unused learned clauses in graph")
	;

	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	boost::program_options::notify(vm);

	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}

	if(vm.count("ignore-mode"))
	{
		unsigned int raw = vm["ignore-mode"].as<int>();
		if(raw > 2)
		{
			std::cout << "ERROR: Ignore mode must be between 0 and 2" << std::endl;
			return 1;
		}
					
		mode = static_cast<ignore_mode>(raw);
	}

	if(vm.count("print-graph"))
	{
		print_graph = true;
		if(vm.count("include-unused")) print_with_unused = true;
	}	


	std::string line;
	SolverShadow solver(mode);

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
			
			solver.num_vars(num);
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

			solver.add_clause(std::make_shared<Clause>(Clause(literals)), ref);
		}
		else if(instruction == "D")
		{
			std::string ls;
			ss >> ls;
			solver.decide(ls);
		}
		else if(instruction == "P")
		{
			std::string ls;
			int ref;
			ss >> ls >> ref;
			solver.propagate(ls, ref);
		}
		else if(instruction == "PU")
		{
			std::string ls;
			ss >> ls;
			Literal l(ls);
			solver.propagate(l);
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

					std::shared_ptr<const Clause> c = solver.clause_by_cref(ref);

					if(to_skip.size() > 0)
					{
						c = solver.skip(ref, to_skip);
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

					assert(remaining->unit() || mode == none);
					assert(remaining->first_literal() == expected_unit || mode == none);
					solver.add_unit(std::make_shared<const Clause>(Clause(*remaining, true)), expected_unit);
					break;
				}
				else if(instruction == "L")
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
					assert(should_be == *remaining.get() || mode == none);

					//if(remaining->is_axiom()) std::cout << "WARNING: learned using only conflict clause" << std::endl;
					solver.add_clause(std::make_shared<const Clause>(Clause(*remaining, true)), ref);
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

					remaining = solver.minimize(remaining, removed_literals);
				}
				else if(instruction == "MNM2")
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

					remaining = solver.minimize_full(remaining, removed_literals);
				}
				else if(instruction == "B")
				{
					int level;
					ss >> level;
					solver.backtrack(level);
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

			solver.backtrack(level);

		}
		else if(instruction == "RS")
		{
			solver.restart();
		}
		else if(instruction == "C")
		{
			int ref;
			ss >> ref;

			ResolutionGraph gb(solver, ref, print_graph);
			if(print_graph)
			{
				if(!print_with_unused) gb.remove_unused();
				gb.print_graphviz();
			}
			else
			{	
				statistics s = gb.vertex_statistics();

				std::cout << "{";
				std::cout << "\"used_axioms\": " << s.used_axioms << ", \"unused_axioms\": " << s.unused_axioms << ",";
				std::cout << "\"used_intermediate\": " << s.used_intermediate << ", \"unused_intermediate\": " << s.unused_intermediate << ",";
				std::cout << "\"used_learned\": " << s.used_learned << ", \"unused_learned\": " << s.unused_learned << ",";

				std::cout << "\"tree_edge_violations\": " << s.tree_edge_violations << ", \"tree_vertex_violations\": " << s.tree_vertex_violations << ",";
				std::cout << "\"tree_copy_cost\": " << s.copy_cost << ", ";

				std::cout << "\"regularity_violations_total\": " << s.regularity_violations_total << ", \"regularity_violation_variables\": " << s.regularity_violation_variables << ",";
				std::cout << "\"max_width\": " << s.width << "}" << std::endl;
			}
			break;
		}
		else if(instruction == "R")
		{
			int ref;
			ss >> ref;
			solver.remove_clause(ref);
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
					solver.relocate(moves);
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
