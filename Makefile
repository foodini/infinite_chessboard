all: infinite_chessboard infinite_chessboard2

infinite_chessboard2: infinite_chessboard2.o util.o
	g++ -O2 -o infinite_chessboard2 -std=c++20 infinite_chessboard2.o util.o
	strip infinite_chessboard

infinite_chessboard: infinite_chessboard.o util.o
	g++ -O2 -o infinite_chessboard -std=c++20 infinite_chessboard.o util.o
	strip infinite_chessboard

infinite_chessboard2.o: infinite_chessboard2.cpp
	g++ -O2 -c -o infinite_chessboard2.o -std=c++20 infinite_chessboard2.cpp

clean:
	rm tmp util.o
	rm infinite_chessboard2.o infinite_chessboard2 infinite_chessboard2 
	rm infinite_chessboard.o infinite_chessboard infinite_chessboard 

net_comms.o: net_comms.h net_comms.cpp
	g++ -O2 -c -o net_comms.o net_comms.cpp

util.o: util.h util.cpp
	g++ -O2 -c -o util.o -std=c++20 util.cpp

tmp: tmp.cpp util.o
  g++ -o tmp -std=c++20 tmp.cpp util.o
