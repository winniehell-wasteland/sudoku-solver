#include <cstring>
#include <iostream>
#include <iterator>

#include "entry.h"
#include "predicates.h"
#include "solver.h"

using namespace std;

// how often does a value occur in the Sudoku
static int* frequencies = 0;
// which values can oocur in the Sudoku
static int* sudoku_values = 0;

// which values are present in column, row, block
static bool** block_values;
static bool** column_values;
static bool** row_values;

static int* ranks;

void print_field(const Entry& entry)
{
  cout << entry.field.first << " " << entry.field.second;
}

void print_sudoku(Sudoku& s)
{
  for(int i = 0; i < s.size(); i++)
  {
    if(i % s.k() == 0)
    {
      cout << "-------------------------" << endl;
    }
    for(int j = 0; j < s.size(); j++)
    {
      if(j % s.k() == 0)
      {
        cout << "| ";
      }

      if(s(j,i) > -1)
      {
        cout << s(j,i) << " ";
      }
      else
      {
        cout << "X ";
      }
    }
    cout << "|" << endl;
  }
  cout << "-------------------------" << endl;
}

bool check_block(Sudoku& s,const Field& field, int value)
{
  for(int i = field.first/s.k()*s.k(); i < (field.first/s.k()+1)*s.k(); i++)
  {
    for(int j = field.second/s.k()*s.k(); j < (field.second/s.k()+1)*s.k(); j++)
    {
      if((i == field.first) && (j == field.second))
      {
        continue;
      }

      // can the value be put in another field?
      if(s.writable(i, j) && !column_values[i][value] && !row_values[j][value])
      {
        return false;
      }
    }
  }

  return true;
}

bool check_column(Sudoku& s, const Entry& entry, int value)
{
  return false;
  
  int j = 0;

  // above block
  for(int j = 0; j < s.size(); j++)
  {
    if(j == entry.field.second)
    {
      continue;
    }
      
    if(s.writable(entry.field.first, j) && !block_values[(entry.block_index % s.k())+j/s.k()*s.k()][value] && !row_values[j][value])
    {
      return false;
    }
  }

  return true;
}

bool check_row(Sudoku& s, const Entry& entry, int value)
{
  for(int i = 0; i < s.size(); i++)
  {
    if(i == entry.field.first)
    {
      continue;
    }
      
    if(s.writable(i, entry.field.second) && !block_values[(entry.block_index/s.k()*s.k())+i/s.k()][value] && !column_values[i][value])
    {
      return false;
    }
  }

  return true;
}

inline bool comp_by_frequency(const int& a, const int& b)
{
  return (frequencies[a]>frequencies[b]);
}

inline bool comp_by_possible_values(const Entry& a, const Entry& b)
{
  if(b.handled)
  {
    return true;
  }
  else if(a.handled)
  {
    return false;
  }
  else
  {
    return (a.num_values < b.num_values);
  }
}

int decrease_possibilities(int sudoku_k, const Entry& entry, int value, EmptyFields& empty_fields, int* changed_fields = 0)
{
  if(empty_fields.empty())
  {
    return 0;
  }
  
  assert(value < sudoku_k*sudoku_k);
  int num_changes = 0;
  
  for(int i = 0; i < entry.num_associations; i++)
  {
    Association& association = entry.associations[i];
    Entry& other = empty_fields[association.first];

    if(other.handled)
    {
      continue;
    }
    
    if(
      ((association.second == 'b') && !column_values[other.field.first][value] && !row_values[other.field.second][value]) ||
      ((association.second == 'c') && !block_values[other.block_index][value] && !row_values[other.field.second][value]) ||
      ((association.second == 'r') && !block_values[other.block_index][value] && !column_values[other.field.first][value])
    )
    {      
      other.num_values--;

      // calculate new rank
      for(; other.rank > 0; other.rank--)
      {
        if(other.num_values < empty_fields[ranks[other.rank-1]].num_values)
        {
          swap(ranks[other.rank], ranks[other.rank-1]);
          empty_fields[ranks[other.rank]].rank = other.rank;
        }
        else
        {
          break;
        }
      }

      if(changed_fields != 0)
      {
        changed_fields[num_changes] = association.first;
        num_changes++;
      }
    }
  }

  assert(num_changes < empty_fields.size());
  
  return num_changes;
}

void insert_value(Sudoku& s, int i, int j, int value)
{
  assert(value < s.size());
  s.set(i,j,value);

  block_values[i/s.k() + j/s.k()*s.k()][value] = true;
  column_values[i][value] = true;
  row_values[j][value] = true;
  
  frequencies[value]++;
}

inline void insert_value(Sudoku& s, const Field& field, int value)
{
  insert_value(s, field.first, field.second, value);
}

void remove_value(Sudoku& s, int i, int j, int value)
{
  s.set(i,j, -1);

  block_values[i/s.k() + j/s.k()*s.k()][value] = false;
  column_values[i][value] = false;
  row_values[j][value] = false;

  frequencies[value]--;
}

inline void remove_value(Sudoku& s, const Field& field, int value)
{
  remove_value(s, field.first, field.second, value);
}

inline void revert_changes(EmptyFields& empty_fields, int* changed_fields, int num_changed_fields)
{  
  for(int j = 0; j < num_changed_fields; j++)
  {
    assert(changed_fields[j] < empty_fields.size());

    Entry& entry = empty_fields[changed_fields[j]];
    
    entry.num_values++;

    // calculate new rank
    for(; entry.rank < empty_fields.size()-1; entry.rank++)
    {
      if(entry.num_values > empty_fields[ranks[entry.rank+1]].num_values)
      {
        swap(ranks[entry.rank], ranks[entry.rank+1]);
        empty_fields[ranks[entry.rank]].rank = entry.rank;
      }
      else
      {
        break;
      }
    }
  }
}

int ratio = 0;

bool sub_solve(Sudoku& s, EmptyFields& empty_fields, int rec_step)
{
  //cout << (100*empty_fields.size()/(s.size()*s.size())) << endl;
  
  // inefficient in lower dimensions
  if((s.k() > 3) && (ratio >= 70))
  {    
    int* changed_fields = new int[3*s.size()];
    int num_changed_fields = 0;

    for(int rank = empty_fields.size()-1; rank > 0; rank --)
    {
      EmptyFields::iterator entry = empty_fields.begin() + ranks[rank];
      
      if(entry->handled)
      {
        continue;
      }
  
      // try all values that have to be placed
      for(int i = 0; i < s.size(); i++)
      {
        if(
          block_values[entry->block_index][i] ||
          column_values[entry->field.first][i] ||
          row_values[entry->field.second][i]
        )
        {
          // value not possible
          continue;
        }

        if(check_block(s, entry->field, i) || check_column(s, *entry, i) || check_row(s, *entry, i))
        {
          num_changed_fields = decrease_possibilities(s.k(), *entry, i, empty_fields, changed_fields);
          insert_value(s, entry->field, i);

          entry->handled = true;

          if(sub_solve(s, empty_fields, rec_step))
          {
            delete[] changed_fields;
            
            return true;
          }
          else
          {
            revert_changes(empty_fields, changed_fields, num_changed_fields);
            remove_value(s, entry->field, i);

            delete[] changed_fields;
            entry->handled = false;

            //cout << "not solvable" << endl;
            return false;
          }
        }
      }

      //break;
    }

    delete[] changed_fields;
  }
  
  EmptyFields::iterator entry = empty_fields.end();

  //entry = min_element(empty_fields.begin(), empty_fields.end(), comp_by_possible_values);

  for(int rank = 0; rank < empty_fields.size(); rank++)
  {
    entry = empty_fields.begin()+ranks[rank];
    
    assert(entry->rank == rank);

    if(!entry->handled)
    {
      break;
    }
  }
  
  // if it is already handled, all empty fields are handled
  if((entry == empty_fields.end()) || entry->handled)
  {    
    //cout << "done" << endl;
    return s.is_solved();
  }

  // check if there are possible values left
  if(entry->num_values == 0)
  {
    //cout << "aborting" << endl;
    return false;
  }

  entry->handled = true;

  assert(s.writable(entry->field.first, entry->field.second));
  
  ++rec_step;
  
  int* changed_fields = new int[3*s.size()];
  int num_changed_fields = 0;

  if(entry->num_values == 1)
  {    
    int value = -1;

    for(int x = 0; x < s.size(); x++)
    {
      if(
        !block_values[entry->block_index][x] &&
        !column_values[entry->field.first][x] &&
        !row_values[entry->field.second][x]
      )
      {
        value = x;
        break;
      }
    }
    
    assert( value > -1 );

    num_changed_fields = decrease_possibilities(s.k(), *entry, value, empty_fields, changed_fields);  
    insert_value(s, entry->field, value);

    if(sub_solve(s, empty_fields, rec_step))
    {
      delete[] changed_fields;
      
      return true;
    }

    revert_changes(empty_fields, changed_fields, num_changed_fields);
    remove_value(s, entry->field, value);
  }
  else
  {
    // try all values that have to be placed
    for(int i = 0; i < s.size(); i++)
    {
      if(
        block_values[entry->block_index][i] ||
        column_values[entry->field.first][i] ||
        row_values[entry->field.second][i]
      )
      {
        // value not possible
        continue;
      }

      if(check_block(s, entry->field, i) || check_column(s, *entry, i) || check_row(s, *entry, i))
      {
        num_changed_fields = decrease_possibilities(s.k(), *entry, i, empty_fields, changed_fields);
        insert_value(s, entry->field, i);

        entry->handled = true;

        if(sub_solve(s, empty_fields, rec_step))
        {
          delete[] changed_fields;

          return true;
        }
        else
        {
          revert_changes(empty_fields, changed_fields, num_changed_fields);
          remove_value(s, entry->field, i);

          delete[] changed_fields;
          entry->handled = false;

          //cout << "not solvable" << endl;
          return false;
        }
      }
    }
    
    int* sorted_values = new int[s.size()];
    memcpy(&sorted_values[0], &sudoku_values[0], s.size()*sizeof(int));
    
    sort(sorted_values, sorted_values+s.size(), comp_by_frequency);

    for(int i = 0; i < s.size(); i++)
    {
      if(
        block_values[entry->block_index][sorted_values[i]] ||
        column_values[entry->field.first][sorted_values[i]] ||
        row_values[entry->field.second][sorted_values[i]]
      )
      {
        // value not possible
        continue;
      }

      num_changed_fields = decrease_possibilities(s.k(), *entry, sorted_values[i], empty_fields, changed_fields);
      assert(sorted_values[i] < s.size());
      insert_value(s, entry->field, sorted_values[i]);

      if(sub_solve(s, empty_fields, rec_step))
      {
        delete[] changed_fields;
        delete[] sorted_values;
        
        return true;
      }
      
      revert_changes(empty_fields, changed_fields, num_changed_fields);
      remove_value(s, entry->field, sorted_values[i]);
    }
    
    delete[] sorted_values;
  }

  delete[] changed_fields;
  entry->handled = false;

  //cout << "backtracking" << endl;
  return false;
}

static int run = 0;

void solve(Sudoku& s)
{
  EmptyFields empty_fields;
  
  //cout << "INITIAL: " << endl;
  //print_sudoku(s);

  frequencies = new int[s.size()];  
  sudoku_values = new int[s.size()];

  for(int x = 0; x < s.size(); x++)
  {
    frequencies[x] = 0;
    sudoku_values[x] = x;
  }

  block_values = new bool*[s.size()];
  block_values[0] = new bool[s.size()*s.size()];
  column_values = new bool*[s.size()];
  column_values[0] = new bool[s.size()*s.size()];
  row_values = new bool*[s.size()];
  row_values[0] = new bool[s.size()*s.size()];
  
  for (int i = 1; i < s.size(); i++)
  {
    block_values[i] = block_values[0] + i * s.size();
    column_values[i] = column_values[0] + i * s.size();
    row_values[i] = row_values[0] + i * s.size();

    for(int j = 0; j < s.size(); j++)
    {      
      block_values[i][j] = false;
      column_values[i][j] = false;
      row_values[i][j] = false;
    }
  }
  
  for(int j = 0; j < s.size(); j++)
  {
    block_values[0][j] = false;
    column_values[0][j] = false;
    row_values[0][j] = false;
  }

  for(int i = 0; i < s.size(); i++)
  {
    for(int j = 0; j < s.size(); j++)
    {
      if(!s.writable(i,j))
      {
        int block_index = i/s.k() + j/s.k()*s.k();
        block_values[block_index][s(i,j)] = true;
        column_values[i][s(i,j)] = true;
        row_values[j][s(i,j)] = true;
        
        frequencies[s(i,j)]++;
      }
    }
  }

  for(int i = 0; i < s.size(); i++)
  {
    for(int j = 0; j < s.size(); j++)
    {
      if(s.writable(i, j))
      {
        Entry entry(s.k(), Field(i,j));
        int first_value = -1;
        
        for(int x = 0; x < s.size(); x++)
        {
          int block_index = i/s.k() + j/s.k()*s.k();
          // check if x is possible value for field
          if(!block_values[block_index][x] && !column_values[i][x] && !row_values[j][x])
          {
            if(first_value == -1)
            {
              first_value = x;
            }
            entry.num_values++;
          }
        }

        assert(entry.num_values > 0);

        if(entry.num_values == 1)
        {
          for(EmptyFields::iterator it = empty_fields.begin(); it != empty_fields.end(); ++it)
          {
            char association = 0;

            if(same_block(*it, entry))
            {
              association = 'b';
            }
            else if(same_column(*it, entry))
            {
              association = 'c';
            }
            else if(same_row(*it, entry))
            {
              association = 'r';
            }

            if(association != 0)
            {
              entry.associate(distance(empty_fields.begin(),it), association);
            }
          }
          
          decrease_possibilities(s.k(), entry, first_value, empty_fields);
          insert_value(s, i, j, first_value);
        }
        else
        {
          empty_fields.push_back(entry);
        }
        
      }
    }
  }

  ranks = new int[empty_fields.size()];

  for(EmptyFields::iterator it = empty_fields.begin(); it != empty_fields.end(); ++it)
  {
    int index = distance(empty_fields.begin(),it);
    
    it->rank = index;
    
    for(; it->rank > 0; it->rank--)
    {
      if(it->num_values < empty_fields[ranks[it->rank-1]].num_values)
      {
        ranks[it->rank] = ranks[it->rank-1];
        empty_fields[ranks[it->rank]].rank = it->rank;
      }
      else
      {
        break;
      }
    }

    ranks[it->rank] = index;
    
    for(EmptyFields::iterator jt = it+1; jt != empty_fields.end(); ++jt)
    {      
      char association = 0;
      
      if(same_block(*it, *jt))
      {
        association = 'b';
      }
      else if(same_column(*it, *jt))
      {
        association = 'c';
      }
      else if(same_row(*it, *jt))
      {
        association = 'r';
      }
      
      if(association != 0)
      {
        it->associate(distance(empty_fields.begin(),jt), association);      
        jt->associate(index, association);
      }
    }
  }

  ratio = 100*empty_fields.size()/(s.size()*s.size());

  //cout << "start recursion" << endl;
  //print_sudoku(s);
  assert(sub_solve(s, empty_fields, 0));
  //print_sudoku(s);

  empty_fields.clear();

  delete[] block_values[0];
  delete[] block_values;
  delete[] column_values[0];
  delete[] column_values;
  delete[] row_values[0];
  delete[] row_values;
  
  delete[] frequencies;
  frequencies = 0;
  
  delete[] sudoku_values;
  sudoku_values = 0;

  delete[] ranks;
}