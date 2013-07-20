#ifndef ENTRY_H
#define ENTRY_H

#include <utility>

typedef std::pair<int,int> Field;
typedef std::pair<int, char> Association;

class Entry
{
public:
  Entry(int sudoku_k, Field field);
  Entry(const Entry& entry);
  ~Entry();

  void associate(int entry_index, char type);

  Association* associations;
  int block_index;
  Field field;
  bool handled;
  int num_associations;
  int num_values;
  int rank;
private:
  int associations_size;
};

#endif // ENTRY_H
