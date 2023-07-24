#include <stdio.h>
#include <map>
#include <set>

#include "util.h"

using namespace std;

class Pos {
public:
  int x;
  int y;

  Pos(const Pos & other) {
    x = other.x;
    y = other.y;
  }
  Pos(int x_arg, int y_arg) {
    x = x_arg;
    y = y_arg;
  }
  Pos() {
    x = y = 0;
  }
  Pos operator+(const Pos & other) const {
    return Pos(x + other.x, y + other.y);
  }
  bool operator==(const Pos & other) const {
    return x == other.x && y == other.y;
  }
  s64 hashval() const {
    return ((s64)x<<32) + y;
  }
  bool operator<(const Pos & other) const {
    return hashval() < other.hashval();
  }
  void print(bool brackets = false, const char * format = "%3d") const {
    if(brackets)
      printf("<");
    printf(format, x);
    printf(",");
    printf(format, y);
    if(brackets)
      printf(">");
  }
};

class Node {
public:
  int val;
  int neighbor_sum;

  Node() {
    val = 0;
    neighbor_sum = 0;
  }

  void print(bool brackets = false, const char * format = "%3d") {
    if(brackets)
      printf("[");
    if(val == 0) {
      if(neighbor_sum == 0) {
        printf("\033[2m");
      } else {
        printf("\033[34;1m");
      }
    } else {
      if(val == 1) {
        printf("\033[31;4m");
      }
    }
    printf(format, val);
    printf(",");
    printf(format, neighbor_sum);
    if(val <= 1)
      printf("\033[0m");
    if(brackets)
      printf("]");
  }
};

class Board {
public:
  map<Pos, Node> squares;
  map<int, set<Pos> > neighbor_sums;

  Board() {
    dxdy[0].x = -1; dxdy[0].y = -1;
    dxdy[1].x = -1; dxdy[1].y =  0;
    dxdy[2].x = -1; dxdy[2].y =  1;
    dxdy[3].x =  0; dxdy[3].y = -1;
    dxdy[4].x =  0; dxdy[4].y =  1;
    dxdy[5].x =  1; dxdy[5].y = -1;
    dxdy[6].x =  1; dxdy[6].y =  0;
    dxdy[7].x =  1; dxdy[7].y =  1;
  }

  void push(const Pos & pos, int val) {
    //There's no check to make sure you're not overwriting something.
    //Don't do it. There's no fixing it once you've broken it.
    squares[pos].val = val;
    for(int i=0; i<8; i++) {
      Pos neighbor = pos + dxdy[i];
      if(squares[neighbor].val == 0) {
        int oldsum = squares[neighbor].neighbor_sum;
        int newsum = oldsum + val;
        squares[neighbor].neighbor_sum = newsum;
        if(oldsum > 0) {
          neighbor_sums[oldsum].erase(neighbor);
        }
        neighbor_sums[newsum].insert(neighbor);
      }
    }
  }

  void pop(const Pos & pos) {
    int val = squares[pos].val;
    squares[pos].val = 0;
    for(int i = 0; i < 8; i++) {
      Pos neighbor = pos + dxdy[i];
      if(squares[neighbor].val == 0) {
        int oldval = squares[neighbor].neighbor_sum;
        int newval = oldval - val;
        squares[neighbor].neighbor_sum = newval;
        neighbor_sums[oldval].erase(neighbor);
        if(newval > 0) {
          neighbor_sums[newval].insert(neighbor);
        }
      }
    }
  }

  void walk() {
    best = 1;
    _walk(2);
  }

  void print() {
    int minx = 99999999;
    int maxx = -minx;
    int miny = 99999999;
    int maxy = -miny;
    for(map<Pos, Node>::iterator i = squares.begin(); i != squares.end(); i++) {
      if(i->first.x > maxx)
        maxx = i->first.x;
      if(i->first.x < minx)
        minx = i->first.x;
      if(i->first.y > maxy)
        maxy = i->first.y;
      if(i->first.y < miny)
        miny = i->first.y;
    }
    for(int y = miny; y <= maxy; y++) {
      for(int x = minx; x <= maxx; x++) {
        Pos xy = Pos(x, y);
        printf("|");
        squares[xy].print();
      }
      printf("|\n");
    }
    int min_sum = 999999999;
    int max_sum = -min_sum;
    for(map<int, set<Pos> >::iterator i=neighbor_sums.begin();
        i!=neighbor_sums.end(); i++) {
      if(i->first < min_sum)
        min_sum = i->first;
      if(i->first > max_sum)
        max_sum = i->first;
    }
    for(int i=min_sum; i<=max_sum; i++) {
      if(neighbor_sums[i].size() == 0) {
        continue;
      }
      /*
      printf("%3d (%3lu elements): ", i, neighbor_sums[i].size());
      for(const Pos & pos_iter : neighbor_sums[i]) {
        pos_iter.print(true);
      }
      printf("\n");
      */
    }
  }

private:
  Pos dxdy[8];
  int best;

  void _walk(int val) {
    // If there's anything that is really going to improve the performance
    // of the walk, it's this. Making this copy is terrible. I should implement
    // my own set that allows you to iterate over a thing that changes while
    // you're in iteration.
    vector<Pos> positions;
    for(const Pos & iter : neighbor_sums[val]) {
      positions.push_back(iter);
    }

    for(const Pos & iter : positions) {
      push(iter, val);
      if(val > best) {
        best = val;
        printf("\n New best: %d\n", val);
        print();
      }
      _walk(val+1);
      pop(iter);
    }
  }
};

int main() {
  Board board;

#warning have you checked to make sure you're not leaking Pos?
  /*
  board.push(Pos(0,0), 1);
  board.push(Pos(2,2), 1);
  */
  board.push(Pos(0,0), 1);
  board.push(Pos(4,2), 1);
  board.push(Pos(6,4), 1);
  board.push(Pos(9,2), 1);

  board.walk();
}
