#include <stdio.h>

#include <format>
#include <set>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "util.h"

/*
TODO:
* unordered_sets should be faster than sets, but a test on walking a single
  four-square Board showed otherwise. It would be worth trying again when there
  are larger boards & deeper traverses going on.
* in _all, we call walk(). walk() should return a bool that indicates whether
  it terminated early due to a symmetry. If this is the case, _all dosen't need
  to call itself - doing so would generate a walk() attempt for every one of the
  symmetric board's subboards - which would each be rejected due to symmetry.
* Make sure that validate() checks the neighbor_sums map. It should exactly
  match current board state.
* Can we move to the cached_neighbor_sum?
*/

#define MARK do{printf("%d\n", __LINE__); fflush(stdout);} while(0)

const u32 BOARD_SIZE = 600;
const u32 BOARD_MID = BOARD_SIZE/2;

using namespace std;

class Pos {
public:
  Pos(const Pos & other) {
    x = other.x;
    y = other.y;
    update_hash_cache();
  }
  Pos(u32 x_arg, u32 y_arg) {
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
  // THIS RETURNS A ROTATED VECTOR!!!!!
  Pos operator^(const Pos & other) const {
    return Pos(y - other.y, other.x - x);
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


// Provides the hashing function for unordered_set of Pos. 
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
  u32 val;
  u32 neighbor_sum;
  u32 cached_neighbor_sum;

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

  Board() {
    one_point_count = 0;
    fill(best_scores, best_scores+10, 0);
    fill(board_counts, board_counts+10, 0);

    fill(number_of_boards, number_of_boards+10, 0);
    number_of_boards[2] = 24;
    number_of_boards[3] = 903;
    number_of_boards[4] = 38288;

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
    one_point_positions.insert(pos);
    one_point_count++;
    _push(pos, 1);
  }

  void pop(const Pos & pos) {
    one_point_positions.erase(pos);
    one_point_count--;
    _pop(pos);
  }

  void walk() {
    if(already_walked()) {
      //printf("He's already got one!\n");
      return;
    }

    _add_to_walked_boards();

    _walk(2);
  }

  void get_bounds(u16 & min_x, u16 & min_y, u16 & max_x, u16 & max_y) {
    min_x = u16_max; max_x = u16_min;
    min_y = u16_max; max_y = u16_min;
    for(auto sets_iter : neighbor_sums) {
      for(auto pos_iter : sets_iter.second) {
        min_x = min(min_x, pos_iter.get_x());
        max_x = max(max_x, pos_iter.get_x());
        min_y = min(min_y, pos_iter.get_y());
        max_y = max(max_y, pos_iter.get_y());
      }
    }
  }

  bool validate() {
    u16 min_x; u16 max_x;
    u16 min_y; u16 max_y;

    get_bounds(min_x, min_y, max_x, max_y);

    for(u32 y=min_y; y<=max_y; y++) {
      for(u32 x=min_x; x<=max_x; x++) {
        //printf("v: <%d, %d>\n", x, y);
        u32 sum = 0;
        u16 val = squares[y][x].val;
        for(s32 dy=-1; dy<=1; dy++) {
          for(s32 dx=-1; dx<=1; dx++) {
            if(dx==0 and dy == 0) {
              continue;
            }
            if(val == 0 or squares[y+dy][x+dx].val < val) {
              sum += squares[y+dy][x+dx].val;
            }
          }
        }
        //printf("sum: %d, neighbor_sum: %d\n", sum, squares[y][x].neighbor_sum);
        if(sum != squares[y][x].neighbor_sum) {
          return false;
        }
      }
    }
    return true;
  }


  void expand(unordered_set<Pos> & expanded, const unordered_set<Pos> & visited) {
    for(auto pos_iter : visited) {
      for(s16 dy=-2; dy<=2; dy++) {
        for(s16 dx=-2; dx<=2; dx++) {
          expanded.insert(Pos(dx + pos_iter.get_x(), dy + pos_iter.get_y()));
        }
      }
    }
  }

  void _all(u32 depth) {
    unordered_set<Pos> to_traverse;
    expand(to_traverse, visited_pos_set);

    for(auto pos_iter : to_traverse) {
      visited_pos_set.clear();
      if(get_square(pos_iter).val != 1) {
        push(pos_iter);
        board_counts[depth]++;
        walk();
        if(depth < max_depth) {
          printf("Calling _all with depth = %d (%d max).\n", depth + 1, max_depth); 
          for(u32 i=2; i<=max_depth; i++) {
            printf("  %d-stone boards processed: %d/%d\n", i, board_counts[i], number_of_boards[i]);
          }
          fflush(stdout);
          _all(depth + 1);
        }
        pop(pos_iter);
      }
    }
  }

  void all() {
    Pos center(BOARD_MID, BOARD_MID);
    push(center);
    max_depth = 3;
    _all(2);

    printf("Stats for each stone count:\n");
    for(u32 i=1; i<=max_depth; i++) {
      printf("%d: best score: %d.  Board count: %d/%d\n", i, best_scores[i],
             board_counts[i], number_of_boards[i]);
    }
  }

  void print() {
    u16 min_x;
    u16 max_x;
    u16 min_y;
    u16 max_y;

    printf("OPP: %ld OPP count: %d, ID: %s\n", one_point_positions.size(),
           one_point_count, compact_repr().c_str());

    get_bounds(min_x, min_y, max_x, max_y);

    for(u32 y = min_y; y <= max_y; y++) {
      for(u32 x = min_x; x <= max_x; x++) {
        Pos xy = Pos(x, y);
        printf("|");
        get_square(xy).print();
      }
      printf("|\n");
    }
    u32 min_sum = u32_max;
    u32 max_sum = u32_min;
    for(unordered_map<u32, set<Pos> >::iterator i=neighbor_sums.begin();
        i!=neighbor_sums.end(); i++) {
      if(i->first < min_sum)
        min_sum = i->first;
      if(i->first > max_sum)
        max_sum = i->first;
    }
    for(u32 i=min_sum; i<=max_sum; i++) {
      if(neighbor_sums[i].size() > 0) {
        //printf("%d: <set tbi>", i);
      }
    }
  }

  string compact_repr() {
    return _compact_repr(one_point_positions);
  }

private:
  Pos dxdy[8];
  Square squares[BOARD_SIZE][BOARD_SIZE];
  unordered_set<Pos> visited_pos_set;
  unordered_set<string> walked_boards;
  unordered_map<u32, set<Pos> > neighbor_sums;
  set<Pos> one_point_positions;
  u32 one_point_count;
  u32 best_scores[10];
  u32 board_counts[10];
  u32 number_of_boards[10];
  u32 max_depth;

  void _push(const Pos & pos, u32 val) {
    visited_pos_set.insert(pos);
    Square & square = get_square(pos);
    square.val = val;
    if(val == 1) {
      neighbor_sums[square.neighbor_sum].erase(pos);
      square.neighbor_sum = 0;
    }
    for(u32 i=0; i<8; i++) {
      Pos neighbor = pos + dxdy[i];
      if(get_square(neighbor).val == 0) {
        u32 oldsum = get_square(neighbor).neighbor_sum;
        u32 newsum = oldsum + val;
        get_square(neighbor).neighbor_sum = newsum;
        if(oldsum > 0) {
          neighbor_sums[oldsum].erase(neighbor);
        }
        neighbor_sums[newsum].insert(neighbor);
      }
    }
  }

  void _pop(const Pos & pos) {
    Square & square = get_square(pos);
    u32 val = square.val;
    if(val == 1) {
      u32 sum = 0;
      for(u32 i=0; i<8; i++) {
        Pos neighbor = pos + dxdy[i];
        sum += squares[neighbor.get_y()][neighbor.get_x()].val;
      }
      squares[pos.get_y()][pos.get_x()].neighbor_sum = sum;

    }
    square.val = 0;
    for(u32 i=0; i<8; i++) {
      Pos neighbor = pos + dxdy[i];
      Square & neighbor_square = get_square(neighbor);
      if(neighbor_square.val == 0) {
        u32 oldval = neighbor_square.neighbor_sum;
        u32 newval = oldval - val;
        neighbor_square.neighbor_sum = newval;
        neighbor_sums[oldval].erase(neighbor);
        if(newval > 0) {
          neighbor_sums[newval].insert(neighbor);
        }
      }
    }
  }

  void _walk(u32 val) {
    // If there's anything that is really going to improve the performance
    // of the walk, it's this. Making this copy is terrible. I should implement
    // my own set that allows you to iterate over a thing that changes while
    // you're in iteration.
    vector<Pos> positions;
    for(const Pos & iter : neighbor_sums[val]) {
      positions.push_back(iter);
    }

    /*
    if(!validate()) {
      printf("VALIDATION FAILURE!!!\n");
      print();
      printf("^^^^^^^^^^^^^^^^^^^^^\n");
    }
    */

    for(const Pos & iter : positions) {
      _push(iter, val);
      if(val > best_scores[one_point_count]) {
        best_scores[one_point_count] = val;
        printf("New best (%d stones): %d  %s\n", one_point_count, val,
               compact_repr().c_str());
        print();
        printf("\n");
      }
      _walk(val+1);
      _pop(iter);
    }
  }

  void _add_to_walked_boards() {
    set<Pos> set_a;
    set<Pos> set_b;
    set<Pos> & cur = set_a;
    set<Pos> & next = set_b;
    set<Pos> & swap_tmp = set_a;

    s16 center_val = 0;

    for(Pos pos_iter : one_point_positions) {
      set_a.insert(pos_iter);

      // This looks weird, but center_pos.x & center_pos.y will be the same,
      // and will be the max of all x and y values of all positions.
      if(pos_iter.get_x() > center_val) {
        center_val = pos_iter.get_x();
      }
      if(pos_iter.get_y() > center_val) {
        center_val = pos_iter.get_y();
      }
    }
    Pos center_pos(center_val, center_val);
    for(u32 reflection = 0; reflection < 2; reflection++) {
      for(u32 rotation = 0; rotation < 2; rotation++) {
        walked_boards.insert(_compact_repr(cur));
        next.clear();
        for(Pos pos_iter : cur) {
          Pos v = pos_iter ^ center_pos;
          next.insert(center_pos + v);
        }
        swap_tmp = next; next = cur; cur = swap_tmp;
      }
      walked_boards.insert(_compact_repr(cur));
      next.clear();
      for(Pos pos_iter : cur) {
        next.insert(Pos(pos_iter.get_y(), pos_iter.get_x()));
      }
      swap_tmp = next; next = cur; cur = swap_tmp;
    }
  }

  //TODO: FASTER!!! I should be formatting directly into the returned string.
  string _compact_repr(const set<Pos> & squares) {
    u32 repr_list[32]; // Not enough compute power in the world to go this big.
    char buf[1024];
    u16 next_in_repr_list = 0;
    u16 min_x = u16_max; u16 max_x = u16_min;
    u16 min_y = u16_max; u16 max_y = u16_min;

    for(auto squares_iter : squares) {
      u16 x = squares_iter.get_x();
      u16 y = squares_iter.get_y();
      min_x = min(min_x, x); max_x = max(max_x, x);
      min_y = min(min_y, y); max_y = max(max_y, y);
    }
    u16 span_x = max_x - min_x;
    u16 span_y = max_y - min_y;

    for(auto squares_iter : squares) {
      u32 x = (u32)(squares_iter.get_x());
      u32 y = (u32)(squares_iter.get_y());

      repr_list[next_in_repr_list] = ((y-min_y)<<16) + (x-min_x);
      next_in_repr_list++;
    }

    sort(repr_list, repr_list + next_in_repr_list);

    u32 len = snprintf(buf, 100, "%xx%x", span_x, span_y);
    char * cur = buf + len;
    for(s32 i=0; i<next_in_repr_list; i++) {
      cur += snprintf(cur, 100, "|%x", repr_list[i]);
    }

    //printf("%s\n", buf);

    return string(buf);
  }

  bool already_walked() {
    return walked_boards.find(compact_repr()) != walked_boards.end();
  }
};

int main() {
  Board board;

  if(false) {
    board.all();
    exit(0);
  }

  board.push(Pos(BOARD_MID + 0,BOARD_MID + 0));
  board.push(Pos(BOARD_MID + 4,BOARD_MID + 2));
  board.push(Pos(BOARD_MID + 6,BOARD_MID + 4));
  board.push(Pos(BOARD_MID + 9,BOARD_MID + 2));
  board.walk();
  board.print();
}
