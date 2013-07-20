#ifndef SUDOKU_H
#define SUDOKU_H

#include <iostream>
#include <cassert>
#include <algorithm>

class Sudoku
{
public:
  Sudoku( int, int* );
  ~Sudoku();

  int operator() ( int i, int j );
  bool writable ( int i, int j );
  void set( int i, int j, int v );

  inline int k( void ) { return k_; }
  inline int size( void ) { return k_*k_; }

  bool is_solved();


private:
  typedef std::pair<int,bool> Entry;
  int k_;
  Entry** data_;
  bool** writable_;
};

#endif // SUDOKU_H
