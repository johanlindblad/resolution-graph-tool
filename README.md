# Resolution-graph-from-minisat-trace (insert fancy name here)

Requires:

* Boost headers (tested with 1.67.0)
* CMake (tested with 3.11.1, likely requires >= 3.1 for C++11 support)
* C++ compiler (anything supporting C++11 should work, tested with Clang)

## Building
1. Create build directory (for example, `mkdir build` from source directory)
2. Run CMake to generate Makefiles (`cmake ..` if using example above, otherwise
`cmake $source_dir`, where `$source_dir` is root source directory)  
For optimized release build, append argument `-DCMAKE_BUILD_TYPE=Release` and for
build with debug symbols append `-DCMAKE_BUILD_TYPE=Debug`.
3. `make`

## Running
1. Pipe minisat trace output to `./ResolutionGraph`.

## Configuring
Top of `main()` contains flags for ignore mode and whether to print GraphViz.

# Interpreting GraphViz output
* Edges are reversed (i.e. resolution gives edges from resulting clause to its
two source clauses). This is because if we want to traverse the graph we will
likely do it in that direction.
* Edge labels are the variable that was removed as a result of the resolution.
* Learned clauses have a turquoise background
* Axiom clauses have a grey background
* Intermediate clauses have a white background
* Clauses that are not used in the final resolution refutation (or equivalently,
are not reachable from the empty clause) are significantly smaller.
