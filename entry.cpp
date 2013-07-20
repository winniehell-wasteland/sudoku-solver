#include <cassert>
#include <cstring>

#include "entry.h"

Entry::Entry(int sudoku_k, Field field):
  field(field), handled(false), num_associations(0), num_values(0), rank(0), associations_size(3*sudoku_k*sudoku_k)
{
  associations = new Association[associations_size];
  block_index = (field.first/sudoku_k + field.second/sudoku_k*sudoku_k);
}

Entry::Entry(const Entry& entry) {
  associations_size = entry.associations_size;
  block_index = entry.block_index;
  field = entry.field;
  handled = entry.handled;
  num_associations = entry.num_associations;
  num_values = entry.num_values;
  rank = entry.rank;

  associations = new Association[associations_size];
  memcpy(&associations[0], &entry.associations[0], num_associations*sizeof(Association));
}

Entry::~Entry()
{
  delete[] associations;
}

void Entry::associate(int entry_index, char type)
{
  assert(num_associations < associations_size);
  associations[num_associations] = Association(entry_index, type);
  num_associations++;
}

