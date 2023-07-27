#include <stdio.h>

#include <format>
#include <unordered_map>
#include <set>
#include <stdlib.h>
#include <string>

#include "util.h"

#define MARK do{printf("%d\n", __LINE__); fflush(stdout);} while(0)
//template<typename T> inline T min(T a, T b) { return a < b ? a : b; }
//template<typename T> inline T max(T a, T b) { return a > b ? a : b; }

const int BOARD_SIZE = 1000;
const int BOARD_MID = BOARD_SIZE/2;

using namespace std;

class Pos {
public:
  Pos(const Pos & other) {
    x = other.x;
    y = other.y;
    update_hash_cache();
  }
  Pos(int x_arg, int y_arg) {
    x = x_arg;
    y = y_arg;
    update_hash_cache();
  }
  Pos() {
    x = y = 0;
    update_hash_cache();
  }
  ~Pos() {
  }

  Pos operator+(const Pos & other) const {
    return Pos(x + other.x, y + other.y);
  }
  bool operator==(const Pos & other) const {
    return x == other.x && y == other.y;
  }
  bool operator<(const Pos & other) const {
    return hash_cache < other.hash_cache;
  }

  //
  // IMPORTANT!!! THIS IS NOT THE SAME AS THE VALUE USED IN THE COMPACT REPR!!!
  void update_hash_cache() {
    hash_cache = ((u64)y<<32) | x;
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
  inline u16 get_x() const { return x; }
  inline u16 get_y() const { return y; }
  inline u64 get_hash() const { return hash_cache; }

private:
  u16 x;
  u16 y;
  u64 hash_cache;
};

namespace std {
  template<>
  struct hash<Pos> {
    typedef Pos argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const {
      return s.get_hash();
    }
  };
}


class Square {
public:
  int val;
  int neighbor_sum;

  Square() {
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
  Square squares[1000][1000];
  unordered_map<int, set<Pos> > neighbor_sums;
  set<Pos> one_point_stones;

  Board() {
    dxdy[0] = Pos(-1, -1);
    dxdy[1] = Pos(-1,  0);
    dxdy[2] = Pos(-1,  1);
    dxdy[3] = Pos( 0, -1);
    dxdy[4] = Pos( 0,  1);
    dxdy[5] = Pos( 1, -1);
    dxdy[6] = Pos( 1,  0);
    dxdy[7] = Pos( 1,  1);
  }

  inline Square & get_square(const Pos & pos) {
    return squares[pos.get_y()][pos.get_x()];
  }

  void push(const Pos & pos) {
    one_point_stones.insert(pos);
    _push(pos, 1);
  }

  void pop(const Pos & pos) {
    one_point_stones.erase(pos);
    _pop(pos);
  }

  void walk() {
    best = 1;
    _walk(2);
  }

  void get_bounds(u16 & min_x, u16 & min_y, u16 & max_x, u16 & max_y) {
    min_x = u16_max;
    max_x = u16_min;
    min_y = u16_max;
    max_y = u16_min;
    for(auto sets_iter : neighbor_sums) {
      for(auto pos_iter : sets_iter.second) {
        min_x = min(min_x, pos_iter.get_x());
        max_x = max(max_x, pos_iter.get_x());
        min_y = min(min_y, pos_iter.get_y());
        max_y = max(max_y, pos_iter.get_y());
      }
    }
  }

  void print() {
    u16 min_x;
    u16 max_x;
    u16 min_y;
    u16 max_y;

    get_bounds(min_x, min_y, max_x, max_y);

    for(int y = min_y; y <= max_y; y++) {
      for(int x = min_x; x <= max_x; x++) {
        Pos xy = Pos(x, y);
        printf("|");
        get_square(xy).print();
      }
      printf("|\n");
    }
    int min_sum = 999999999;
    int max_sum = -min_sum;
    for(unordered_map<int, set<Pos> >::iterator i=neighbor_sums.begin();
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
    }
  }

  //TODO: FASTER!!! I should be formatting directly into the returned string.
  string get_compact_repr() {
    u32 repr_list[32]; // Not enough compute power in the world to go this big.
    char buf[1024];
    u16 next_in_repr_list = 0;
    u16 min_x = u16_max;
    u16 max_x = u16_min;
    u16 min_y = u16_max;
    u16 max_y = u16_min;

    for(auto one_point_stones_iter : one_point_stones) {
      u16 x = one_point_stones_iter.get_x();
      u16 y = one_point_stones_iter.get_y();
      min_x = min(min_x, x);
      max_x = max(max_x, x);
      min_y = min(min_y, y);
      max_y = max(max_y, y);
    }
    u16 span_x = max_x - min_x;
    u16 span_y = max_y - min_y;

    for(auto one_point_stones_iter : one_point_stones) {
      u32 x = (u32)(one_point_stones_iter.get_x());
      u32 y = (u32)(one_point_stones_iter.get_y());

      repr_list[next_in_repr_list] = ((y-min_y)<<16) + (x-min_x);
      next_in_repr_list++;
    }

    sort(repr_list, repr_list + next_in_repr_list);

    u32 len = snprintf(buf, 100, "%x%x", span_x, span_y);
    char * cur = buf + len;
    for(s32 i=0; i<next_in_repr_list; i++) {
      cur += snprintf(cur, 100, "|%x", repr_list[i]);
    }

    return string(buf);
  }

private:
  Pos dxdy[8];
  int best;

  void _push(const Pos & pos, int val) {
    //There's no check to make sure you're not overwriting something.
    //Don't do it. There's no fixing it once you've broken it.
    get_square(pos).val = val;
    for(int i=0; i<8; i++) {
      Pos neighbor = pos + dxdy[i];
      if(get_square(neighbor).val == 0) {
        int oldsum = get_square(neighbor).neighbor_sum;
        int newsum = oldsum + val;
        get_square(neighbor).neighbor_sum = newsum;
        if(oldsum > 0) {
          neighbor_sums[oldsum].erase(neighbor);
        }
        neighbor_sums[newsum].insert(neighbor);
      }
    }
  }

  void _pop(const Pos & pos) {
    Square & pos_square = get_square(pos);
    int val = pos_square.val;
    pos_square.val = 0;
    for(int i = 0; i < 8; i++) {
      Pos neighbor = pos + dxdy[i];
      Square & neighbor_square = get_square(neighbor);
      if(neighbor_square.val == 0) {
        int oldval = neighbor_square.neighbor_sum;
        int newval = oldval - val;
        neighbor_square.neighbor_sum = newval;
        neighbor_sums[oldval].erase(neighbor);
        if(newval > 0) {
          neighbor_sums[newval].insert(neighbor);
        }
      }
    }
  }

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
      _push(iter, val);
      if(val > best) {
        best = val;
        printf("\n New best: %d\n", val);
        print();
      }
      _walk(val+1);
      _pop(iter);
    }
  }
};

int main() {
  Board board;

  /*
  board.push(Pos(0,0));
  board.push(Pos(2,2));
  */
  board.push(Pos(BOARD_MID + 0, BOARD_MID + 0));
  board.push(Pos(BOARD_MID + 4, BOARD_MID + 2));
  board.push(Pos(BOARD_MID + 6, BOARD_MID + 4));
  board.push(Pos(BOARD_MID + 9, BOARD_MID + 2));

  board.print();

  board.walk();

}
