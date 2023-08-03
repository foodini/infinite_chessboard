all: infinite_chessboard2.o util.o
  # DON'T FORGET to strip the executable when creating a release version
	g++ -g -o infinite_chessboard -std=c++20 infinite_chessboard2.o util.o

infinite_chessboard2.o: infinite_chessboard2.cpp
	g++ -g -c -o infinite_chessboard2.o -std=c++20 infinite_chessboard2.cpp

clean:
	rm util.o infinite_chessboard.o infinite_chessboard infinite_chessboard2

util.o: util.h util.cpp
	g++ -c -o util.o -std=c++20 util.cpp

tmp: tmp.cpp util.o
  g++ -o tmp -std=c++20 tmp.cpp util.o
