all: infinite_chessboard.o util.o
	g++ -o infinite_chessboard -std=c++20 infinite_chessboard.o util.o

infinite_chessboard.o: infinite_chessboard.cpp
	g++ -c -o infinite_chessboard.o -std=c++20 infinite_chessboard.cpp

clean:
	rm util.o infinite_chessboard.o infinite_chessboard

util.o: util.h util.cpp
	g++ -c -o util.o util.cpp
