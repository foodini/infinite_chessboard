all: infinite_chessboard.cpp util.o
	g++ -o infinite_chessboard -std=c++11 infinite_chessboard.o util.o

infinite_chessboard.o:
	g++ -c -o infinite_chessboard.o infinite_chessboard.cpp

clean:
	rm util.o infinite_chessboard

util.o: util.h util.cpp
	g++ -c -o util.o util.cpp
