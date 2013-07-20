/*/true
g++ -Wall -I/usr/local/ilog/cplex121/include/ilcplex/ -L/usr/local/ilog/cplex121/lib/x86-64_darwin8_gcc4.0/static_pic/ -lcplex  -m64 -lm -framework CoreFoundation -framework IOKit  main.cpp
# -shared-intel  -lpthread main.cpp
exit
 */
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cplex.h>

using namespace std;

class Sudoku
{
public:
  Sudoku( int, int* );
  ~Sudoku();

  int operator() ( int i, int j );
  bool writable( int i, int j );
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











































/* Der Teil ab hier ist durch eine Methode solve(Sudoku&) zu ersetzen, die ein Sudoku direkt löst */

CPXENVptr cpxenv_;

void cplex_error( const std::string& d, int s )
{
  char buf[4096];
  const char* emsg = CPXgeterrorstring(cpxenv_, s, buf);
  if( emsg==0 )
    cerr << d << ": error #" << s << endl;
  else
    cerr << d << ": " << emsg << endl;
}


inline int nvars( Sudoku& s )
{
  return s.size()*s.size()*s.size();
}

inline int varid( Sudoku& s, int i, int j, int v )
{
  return ( i*s.size()*s.size() ) + ( j*s.size() ) + v;
}

bool solve( Sudoku& s )
{
  bool res;
  int status;
  
  CPXLPptr  cpxlp = CPXcreateprob(cpxenv_, &status,"ip");
  if( cpxlp == 0 ) 
    { cplex_error( "CPXcreateprob", status ); abort(); }

  if( (status=CPXchgprobtype(cpxenv_,cpxlp,CPXPROB_MILP)) )
    { cplex_error( "CPXchgprobtype", status ); abort(); }


  char* ctypes = new char[ nvars(s) ];
  double* lb = new double[ nvars(s) ];

  std::fill( ctypes, ctypes+nvars(s), 'B' );
  for( int row=0; row<s.size(); ++row )
    for( int col=0; col<s.size(); ++col )
	for( int val=0; val<s.size(); ++val )
	  lb[ varid(s,row,col,val) ] =   ( s(row,col)==val ? 1.0 : 0.0 );
  if( (status=CPXnewcols(cpxenv_,cpxlp,
			 nvars(s),
			 0,lb,0,ctypes,0 )))
    { cplex_error( "CPXnewcols", status ); abort(); }


  int rowid = 0;
  double rhs = 1;
  
  // cells
  for( int row=0; row<s.size(); ++row )
    for( int col=0; col<s.size(); ++col )
      {
	if((status = CPXnewrows( cpxenv_, cpxlp, 1, &rhs, "E", 0,0 ) )) { cplex_error( "CPXnewrows", status ); abort(); }
	for( int val=0; val<s.size(); ++val )
	  if((status=CPXchgcoef( cpxenv_, cpxlp, rowid, varid(s,row,col,val), 1.0 )))
	    {  cplex_error( "CPXchgcoef", status ); abort(); }
	++rowid;
      }

  // rows?
  for( int row=0; row<s.size(); ++row )
    for( int val=0; val<s.size(); ++val )
      {
	if((status = CPXnewrows( cpxenv_, cpxlp, 1, &rhs, "E", 0,0 ) )) { cplex_error( "CPXnewrows", status ); abort(); }
	for( int col=0; col<s.size(); ++col )
	  if((status=CPXchgcoef( cpxenv_, cpxlp, rowid, varid(s,row,col,val), 1.0 )))
	    {  cplex_error( "CPXchgcoef", status ); abort(); }
	++rowid;
      }

  // cols?
  for( int col=0; col<s.size(); ++col )
    for( int val=0; val<s.size(); ++val )
      {
	if((status = CPXnewrows( cpxenv_, cpxlp, 1, &rhs, "E", 0,0 ) )) { cplex_error( "CPXnewrows", status ); abort(); }
	for( int row=0; row<s.size(); ++row )
	  if((status=CPXchgcoef( cpxenv_, cpxlp, rowid, varid(s,row,col,val), 1.0 )))
	    {  cplex_error( "CPXchgcoef", status ); abort(); }
	++rowid;
      }

  // blocks
  for( int brow=0; brow<s.size(); brow += s.k() )
    for( int bcol=0; bcol<s.size(); bcol += s.k() )
      for( int val=0; val<s.size(); ++val )
	{
	  if((status = CPXnewrows( cpxenv_, cpxlp, 1, &rhs, "E", 0,0 ) )) { cplex_error( "CPXnewrows", status ); abort(); }
	  for( int row=0; row<s.k(); ++row )
	    for( int col=0; col<s.k(); ++col )
	      if((status=CPXchgcoef( cpxenv_, cpxlp, rowid, varid(s,row+brow,col+bcol,val), 1.0 )))
		{  cplex_error( "CPXchgcoef", status ); abort(); }
	  ++rowid;
	}
  
  if( (status=CPXmipopt(cpxenv_,cpxlp)))
    { cplex_error( "CPXmipopt", status ); abort(); }



  int lpstatus;
  double* sol = new double[ nvars(s) ]; 
  status = CPXsolution( cpxenv_, cpxlp,
			&lpstatus, 0,
			sol, 0,
			0,0 );

  if( status == CPXERR_NO_SOLN )
    {
      res = false;
    }
  else 
    {
      if( status != 0 )
	{ cplex_error( "CPXsolution", status ); abort(); }

	
      if( lpstatus != CPXMIP_OPTIMAL )
	{
	  cerr << endl << "solution status is not CPXMIP_OPTIMAL, but " << lpstatus << endl;
	  abort();
	}

      for( int row=0; row<s.size(); ++row )
	for( int col=0; col<s.size(); ++col )
	  if( s.writable( row, col ) )
	    for( int val=0; val<s.size(); ++val )
	      if( sol[ varid(s,row,col,val) ] >= .99 )
		s.set( row, col, val );
      res = true;
    }

  delete [] ctypes;
  delete [] lb;
  delete [] sol;

  if( (status=CPXfreeprob(cpxenv_,&cpxlp)))
    { cplex_error( "CPXfreeprob", status ); abort(); }

  return res;
}

/* Ende des zu ersetzenden Teils */



























#include "testset_sudokus.cpp"

int main( void )
{
  int status;
  cpxenv_ = CPXopenCPLEX(&status);
  if (cpxenv_ == 0) {
    cplex_error( "CPXopenCPLEX()", status );
    abort();
  }
  

  init_testset();
  test_all();

  return 0;
}
