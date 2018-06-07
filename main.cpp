#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include "literal.hpp"
#include "clause.hpp"
#include "resolution_graph.hpp"

int main()
{
	std::string line;
	ResolutionGraph r;
	while(std::getline(std::cin, line))
	{
		std::istringstream ss(line);

		std::string instruction;
		ss >> instruction;

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
		else if(instruction == "U")
		{
			std::vector<Literal> empty = {};
			Clause remaining(empty);

			while(true)
			{
				if(instruction == "U")
				{
					int ref;
					ss >> ref;
					std::shared_ptr<Clause> c = r.clause_by_cref(ref);
					Clause intermediate = remaining.resolve_with(*c.get());
					remaining = intermediate;
				}
				else if(instruction == "LU")
				{
					std::string l;
					ss >> l;
					assert(remaining.unit());
					assert(remaining.first_literal() == Literal(l));

					r.add_unit(std::make_shared<Clause>(remaining));
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

					assert(should_be == remaining);

					r.add_clause(std::make_shared<Clause>(remaining), ref);
					break;
				}
				else
				{
					assert(instruction == "");
				}

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
		else
		{
			std::cout << instruction << std::endl;
		}
	}
}
