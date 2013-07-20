#ifndef SOLVER_H_INCLUDED
#define SOLVER_H_INCLUDED

#include <algorithm>
#include <vector>

#include "entry.h"
#include "sudoku.h"

typedef std::vector<Entry> EmptyFields;

void solve(Sudoku& s);

#endif // SOLVER_H_INCLUDED
