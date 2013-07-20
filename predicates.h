#ifndef PREDICATES_H_INCLUDED
#define PREDICATES_H_INCLUDED

#include "sudoku.h"

/*
bool same_block(int sudoku_k, const Field& a, const Field& b)
{
  return (block_index(sudoku_k, a) == block_index(sudoku_k, b));
}
*/

inline bool same_block(const Entry& a, const Entry& b)
{
  return (a.block_index == b.block_index);
}

inline bool same_column(const Field& a, const Field& b)
{
  return (a.first == b.first);
}

inline bool same_column(const Entry& a, const Entry& b)
{
  return same_column(a.field, b.field);
}

inline bool same_row(const Field& a, const Field& b)
{
  return (a.second == b.second);
}

inline bool same_row(const Entry& a, const Entry& b)
{
  return same_row(a.field, b.field);
}

#endif // PREDICATES_H_INCLUDED