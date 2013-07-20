#include <iostream>

#include "solver.h"
#include "sudoku.h"
#include "testset_sudokus.h"

using namespace std;

int main()
{
  init_testset();
  test_all();
  //test_2();
  //test_3();
  //test_4();
}
