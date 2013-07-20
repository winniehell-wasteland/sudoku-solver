#include <iostream>
#include <cassert>
#include <algorithm>
#include <utility>

#include "sudoku.h"

using namespace std;

Sudoku::Sudoku( int k, int* d )
  : k_ ( k )
{
  data_ = new Entry* [ size() ];
  for( int i=0; i<size(); ++i )
    data_[i] = new Entry[size()];

  for( int row=0; row<size(); ++row )
    for( int col=0; col<size(); ++col )
      data_[row][col] = make_pair( d[(row*k*k) + col], d[(row*k*k) + col]<0 );
}

Sudoku::~Sudoku()
{
  for( int i=0; i<size(); ++i )
    delete[] data_[i];
  delete[] data_;
}

int Sudoku::operator() ( int i, int j )
{
  assert( i>=0 && i<size() );
  assert( j>=0 && j<size() );
  return data_[i][j].first;
}

bool Sudoku::writable( int i, int j )
{
  assert( i>=0 && i<size() );
  assert( j>=0 && j<size() );
  return data_[i][j].second;
}

void Sudoku::set ( int i, int j, int v )
{
  assert( i>=0 && i<size() );
  assert( j>=0 && j<size() );
  assert( data_[i][j].second );
  data_[i][j].first = v;
}


bool Sudoku::is_solved( void )
{


  // rows?
  for( int row=0; row<size(); ++row )
    for( int val=0; val<size(); ++val )
      {
  int cnt=0;
  for( int col=0; col<size(); ++col )
    if( data_[row][col].first == val )
      ++cnt;
  if( cnt != 1 ) return false;
      }

  // cols?
  for( int col=0; col<size(); ++col )
    for( int val=0; val<size(); ++val )
      {
  int cnt=0;
  for( int row=0; row<size(); ++row )
    if( data_[row][col].first == val )
      ++cnt;
  if( cnt != 1 ) return false;
      }

  // blocks
  for( int brow=0; brow<size(); brow += k_ )
    for( int bcol=0; bcol<size(); bcol += k_ )
      for( int val=0; val<size(); ++val )
  {
    int cnt=0;
    for( int row=0; row<k_; ++row )
      for( int col=0; col<k_; ++col )
        if( data_[brow+row][bcol+col].first == val )
    ++cnt;
    if( cnt != 1 ) return false;
  }

  return true;
}
