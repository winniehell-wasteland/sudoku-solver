PROJECT(sudoku)

# gprof
#SET(CMAKE_CXX_FLAGS "-g -pg")

# valgrind
#SET(CMAKE_CXX_FLAGS "-g -O0")

#SET(CMAKE_CXX_FLAGS "-g0 -O3 ") #-DNDEBUG

ADD_EXECUTABLE(sudoku entry.cpp main.cpp solver.cpp sudoku.cpp testset_sudokus.cpp)

