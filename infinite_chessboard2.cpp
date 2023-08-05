#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <string>
#include <thread>
#include <unordered_set>

#include "util.h"

#define MARK do{printf("%d\n", __LINE__); fflush(stdout);}while(0)

/*
KNOWN:
Stone Count | Best Score | Num Boards | Runtime
          2 |         16 | 5          |
          3 |         28 | 113        |
          4 |         38 | 11284      |
          5 |         49 |            | 13h06m34s
          6 |         60 |
*/

#define DEFINE_INSERT(list_name) \
  void list_name##_insert(Square * square) { \
    if(square->list_name##_valid == true) return; /*DON'T RE-INSERT!!!*/ \
    square->list_name##_valid = true; \
    square->list_name##_next = this->list_name##_next; \
    square->list_name##_prev = this; \
    square->list_name##_next->list_name##_prev = square; \
    this->list_name##_next = square; \
  }
#define DEFINE_ERASE(list_name) \
  void list_name##_erase() { \
    if(this->list_name##_valid == false) return; \
    this->list_name##_valid = false; \
    this->list_name##_prev->list_name##_next = this->list_name##_next; \
    this->list_name##_next->list_name##_prev = this->list_name##_prev; \
    this->list_name##_prev = this->list_name##_next = NULL; \
  }
#define ITERATE_INDEX(list_name, index, iterator_name) \
  for(Square * iterator_name = list_name##_list[index].list_name##_next; \
      iterator_name->list_name##_next != NULL; \
      iterator_name = iterator_name->list_name##_next)
class Square {
private:
  u16 x;
  u16 y;
  u16 val;
  u16 neighbor_sum;
  u16 cached_neighbor_sum; //TODO

  Square * neighbor_sums_next;
  Square * neighbor_sums_prev;
  bool neighbor_sums_valid; 
  Square * one_point_squares_next;
  Square * one_point_squares_prev;
  bool one_point_squares_valid; 
  Square * visited_next;
  Square * visited_prev;
  bool visited_valid; 

  DEFINE_INSERT(one_point_squares);
  DEFINE_ERASE(one_point_squares);
  DEFINE_INSERT(neighbor_sums);
  DEFINE_ERASE(neighbor_sums);
  DEFINE_INSERT(visited);
  DEFINE_ERASE(visited);

public:
  Square() :
      val(0),
      neighbor_sum(0),
      cached_neighbor_sum(0),
      neighbor_sums_next(NULL),
      neighbor_sums_prev(NULL),
      one_point_squares_prev(NULL),
      one_point_squares_next(NULL),
      neighbor_sums_valid(false),
      one_point_squares_valid(false),
      visited_valid(false)
  {
  }

  void print() {
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
    printf("%3d,%3d", val, neighbor_sum);
    if(val <= 1)
      printf("\033[0m");
  }

  friend class Board;
};

#define ITERATE(list_name, iterator_name) \
  for(Square * iterator_name = list_name##_list.list_name##_next; \
      iterator_name->list_name##_next != NULL; \
      iterator_name = iterator_name->list_name##_next)

class Board {
private:
  static const u32 board_size = 1001; // MUST BE ODD (so refletion works)
  static const u32 board_mid = board_size/2;
  static const u32 max_neighbor_sums = 2000; //Paying memory for safety/speed.
  static const u16 max_depth = 5;
  static const u32 buf_len = 16*(max_depth + 1); //A generous estimate.

  Square squares[board_size][board_size];
  char packed_repr_buffs[8][buf_len];
  std::unordered_set<std::string> walked_boards;
  u16 best_scores[max_depth + 1];
  std::vector<std::string> best_solutions;
  u64 total_board_counts[9] = {0, 0, 5, 113, 11284, 940000, 0, 0, 0};
  u64 checked_board_counts[max_depth + 1];

  // It's highly unusual to keep linked lists this way, with guards at either
  // end of the list. However, I'm shooting for a fast run here, so I want to
  // avoid inserts and deletes with special cases. This way, insert and delete
  // have no ifs, so code is simpler and no chance of a pipeline stall.
  Square neighbor_sums_list[max_neighbor_sums];
  Square neighbor_sums_ends[max_neighbor_sums];
  Square one_point_squares_list;
  Square one_point_squares_end;
  Square visited_list;
  Square visited_end;

  u32 one_point_count;

  s16 dydx[8][2] = {
    {-1,-1}, {-1, 0}, {-1, 1},
    { 0,-1},          { 0, 1},
    { 1,-1}, { 1, 0}, { 1, 1}};

  void get_extents(u16 & min_x, u16 & min_y, u16 & max_x, u16 & max_y) {
    min_x = u16_max, max_x = u16_min;
    min_y = u16_max, max_y = u16_min;

    ITERATE(visited, iter) {
      min_x = std::min(min_x, iter->x);
      max_x = std::max(max_x, iter->x);
      min_y = std::min(min_y, iter->y);
      max_y = std::max(max_y, iter->y);
    }
  }

  void _push(u16 x, u16 y, u32 val) {
    //printf("_push(x=%d, y=%d, val=%d)\n", x, y, val);
    Square * square = &squares[y][x];
    visited_list.visited_insert(square);
    square->val = val;
    for(u32 i=0; i<8; i++) {
      u16 neighbor_y = y + dydx[i][0];
      u16 neighbor_x = x + dydx[i][1];
      Square * neighbor_square = &squares[neighbor_y][neighbor_x];
      if(neighbor_square->val == 0) {
        u32 old_sum = neighbor_square->neighbor_sum;
        u32 new_sum = old_sum + val;
        neighbor_square->neighbor_sum = new_sum;
        if(old_sum > 0) {
          neighbor_square->neighbor_sums_erase();
        }
        neighbor_sums_list[new_sum].neighbor_sums_insert(neighbor_square);
      }
    }
  }

  void _pop(u16 x, u16 y) {
    Square * square = &squares[y][x];
    u16 val = square->val;

    square->val = 0;
    for(u32 i=0; i<8; i++) {
      u16 neighbor_y = y + dydx[i][0];
      u16 neighbor_x = x + dydx[i][1];
      Square * neighbor_square = &squares[neighbor_y][neighbor_x];
      if(neighbor_square->val == 0) {
        u16 old_val = neighbor_square->neighbor_sum;
        u16 new_val = old_val - val;
        neighbor_square->neighbor_sum = new_val;
        neighbor_square->neighbor_sums_erase();
        if(new_val > 0) {
          neighbor_sums_list[new_val].neighbor_sums_insert(neighbor_square);
        }
      }
    }
  }

  inline u32 pack(u16 x, u16 y, u16 min_x, u16 min_y) {
    return ((y-min_y)<<16) + (x-min_x);
  }

  inline void unpack(u32 packed, u16 & x, u16 & y, u16 min_x, u16 min_y) {
    y = (packed>>16) + min_y;
    x = (packed&0xffff) + min_x;
    printf("packed&0xffff: %d\n", packed&0xffff);
    printf("returning x, y: %d %d\n", x, y);
  }

  inline void u32_to_buf(
      char buf[], u32 repr_list[], u32 count, u16 span_x, u16 span_y) {
    std::sort(repr_list, repr_list + count);

    //TODO: faster to format straight into the output string?
    u32 len = snprintf(buf, buf_len, "%xx%x", span_x, span_y);
    for(u32 i=0; i<count; i++) {
      // I'm not counting total 
      len += snprintf(buf+len, buf_len-len, "|%x", repr_list[i]);
    }
  }

  //Returns true if first string is already in walked_board_set.
  //Setting do_all to true forces the generation of all strings. The
  //values of strings1-7 are garbage if do_all==false and retval==false.
  bool check_and_update_walked_set(bool do_all=false, bool skip_update=false) {
    u32 repr_list[8][max_depth];
    u32 repr_list_next=0;
    u16 min_x, min_y, max_x, max_y;

    get_extents(min_x, min_y, max_x, max_y);
    u16 span_x = 1 + max_x - min_x;
    u16 span_y = 1 + max_y - min_y;
    ITERATE(one_point_squares, iter) {
      u32 x = (u32)(iter->x);
      u32 y = (u32)(iter->y);

      repr_list[0][repr_list_next++] = pack(x, y, min_x, min_y);
    }

    u32_to_buf(
      packed_repr_buffs[0], repr_list[0], repr_list_next, span_x, span_y);

    //TODO: If the first one is already in the cache, quit early.

    std::string str_of_buf(packed_repr_buffs[0]);
    if(!do_all && walked_boards.find(str_of_buf) != walked_boards.end()) {
      return true;
    }
    if(!skip_update) {
      walked_boards.insert(str_of_buf);
    }

    /*
    As the saying goes, the algorithm to do this is very nasty. In fact,
    you might want to mug someone with it. Let's say that we start with 
    the three stones labeled 0 on a 15x15 board:

      0123456789abcde
    0 +++++++++++++++
    1 ++++66+++77++++
    2 ++++++6+7++++++
    3 +++++++++++++++
    4 +1+++++++++++2+
    5 +1+++++++++++2+
    6 ++1+++++++++2++
    7 +++++++++++++++
    8 ++0+++++++++3++
    9 +0+++++++++++3+
    a +0+++++++++++3+
    b +++++++++++++++
    c ++++++5+4++++++
    d ++++55+++44++++

    min_x=1, min_y=8, max_x=2, max_y=10.

    As we go through the 8 permutations of these three stones, the smallest
    x in the triple will sometimes be min_x (rotations 0 and 1), but it may
    also be min_y (like in rotations 4 and 7), or the reflections of max_x
    or max_y. In rotations 2 and 3, the min x we need for computing offsets
    in pack() is 12, which is the reflection (or "inverse") of max_x.

    Maybe there's a way to pack this into a loop inside of ITERATE, but I
    didn't see it.
    */

    repr_list_next = 0;
    u16 max_x_inv = board_size - max_x - 1;
    u16 max_y_inv = board_size - max_y - 1;
    ITERATE(one_point_squares, iter) {
      u16 x = iter->x;
      u16 y = iter->y;
      //We've already done [y][x], so we output nothing to repr_list[0].
      //Let's reflect about y=board_mid:
      y = board_size - y - 1;
      repr_list[1][repr_list_next] = pack(x, y, min_x, max_y_inv);
      //Reflect about x=board_mid axis:
      x = board_size - x - 1;
      repr_list[2][repr_list_next] = pack(x, y, max_x_inv, max_y_inv);
      //Reflect about y=board_mid again:
      y = board_size - y - 1;
      repr_list[3][repr_list_next] = pack(x, y, max_x_inv, min_y);

      //Now we do a reflection about x=y:
      std::swap(x, y);
      repr_list[4][repr_list_next] = pack(x, y, min_y, max_x_inv);

      //Reflect about x=board_mid (continuing clockwise.):
      x = board_size - x - 1;
      repr_list[5][repr_list_next] = pack(x, y, max_y_inv, max_x_inv);
      //Reflect about y=board_mid
      y = board_size - y - 1;
      repr_list[6][repr_list_next] = pack(x, y, max_y_inv, min_x);
      //Reflect about x=board_mid
      x = board_size - x - 1;
      repr_list[7][repr_list_next] = pack(x, y, min_y, min_x);

      repr_list_next++;
    }
    for(u32 i=1; i<8; i++) {
      //TODO: is it faster to do two i loops w/o the if, or is the optimizer
      //      getting it?
      if(i<4) {
        u32_to_buf(
          packed_repr_buffs[i], repr_list[i], repr_list_next, span_x, span_y);
      } else {
        u32_to_buf(
          packed_repr_buffs[i], repr_list[i], repr_list_next, span_y, span_x);
      }

      if(!skip_update) {
        walked_boards.insert(std::string(packed_repr_buffs[i]));
      }
    }

    return false;
  }

  void _walk(u16 val) {
    // I hate doing this copy. I'd love to find a way to skip it.
    // TODO: replace with an iterator macro.
    std::vector<Square *> neighbor_sums_equal_to_val;
    ITERATE_INDEX(neighbor_sums, val, iter) {
      neighbor_sums_equal_to_val.push_back(iter);
    }

    for(Square * square : neighbor_sums_equal_to_val) {
      _push(square->x, square->y, val);
      if(val > best_scores[one_point_count]) {
        best_scores[one_point_count] = val;
        printf("New best (%d stones): %d\n", one_point_count, val);
        print(true, false, false);
        best_solutions[one_point_count] = packed_repr_buffs[0];
        printf("\n");
      }
      _walk(val+1);
      _pop(square->x, square->y);
    }
  }

  void refresh_visited_list() {
    ITERATE(visited, square) {
      square->visited_valid = false;
    }

    visited_list.visited_next = &(visited_end);
    visited_end.visited_prev = &(visited_list);

    ITERATE(one_point_squares, square) {
      for(s16 dy=-1; dy<=1; dy++) {
        for(s16 dx=-1; dx<=1; dx++) {
          u16 x = square->x + dx;
          u16 y = square->y + dy;
          //printf("adding <%d, %d> back to visited list\n", dx, dy);
          visited_list.visited_insert(&squares[y][x]);
        }
      }
    }
  }

  void report_counts() {
    printf("\n");
    for(u32 i=2; i<=max_depth; i++) {
      printf("%d stone best: %d, checked: %lu/%lu, solution: %s\n",
          i, best_scores[i], checked_board_counts[i], total_board_counts[i],
          best_solutions[i].c_str());
    }
  }

public:
  Board() :
      one_point_count(0)
  {
    for(u32 y=0; y<board_size; y++) {
      for(u32 x=0; x<board_size; x++) {
        squares[y][x].x = x;
        squares[y][x].y = y;
      }
    }
    std::fill(best_scores, best_scores + max_depth + 1, 0);
    std::fill(checked_board_counts, checked_board_counts + max_depth + 1, 0);
    best_solutions.reserve(max_depth+1);

    one_point_squares_list.one_point_squares_next = &one_point_squares_end;
    one_point_squares_end.one_point_squares_prev = &one_point_squares_list;

    visited_list.visited_next = &(visited_end);
    visited_end.visited_prev = &(visited_list);

    for(u32 i=0; i<max_neighbor_sums; i++) {
      neighbor_sums_list[i].neighbor_sums_next = &(neighbor_sums_ends[i]);
      neighbor_sums_ends[i].neighbor_sums_prev = &(neighbor_sums_list[i]);
    }
  }

  Board(const std::string & state) :
      Board()
  {
    u16 width, height;
    std::vector<std::string> fields = split(state, '|');
    std::vector<std::string> dimensions = split(fields[0], 'x');
    width = std::stoi(dimensions[0].c_str(), NULL, 10);
    height = std::stoi(dimensions[1].c_str(), NULL, 10);
    u16 min_x = board_mid - width/2;
    u16 min_y = board_mid - height/2;
    u16 x, y;
    for(std::vector<std::string>::iterator i = ++fields.begin();
        i != fields.end(); i++) {
      unpack(std::stoi(i->c_str(), NULL, 16), x, y, min_x, min_y);
      push(x, y);
    }
  }

  Board(const char * state) :
      Board(std::string(state))
  {
  }

  void walk() {
    _walk(2);
  }

  void all() {
    push(board_mid, board_mid);
    for(u16 dy=0; dy<=2; dy++) {
      for(u16 dx=0; dx<=2; dx++) {
        if(dy>0 || dx>0) {
          push(board_mid + dx, board_mid + dy);
          _all(2);
          pop(board_mid + dx, board_mid + dy);
        }
      }
    }
    pop(board_mid, board_mid);
    report_counts();
  }

  //TODO: move to private:
  void _expand(std::unordered_set<Square *> & expanded) {
    ITERATE(visited, square) {
      for(s16 dy=-2; dy<=2; dy++) {
        for(s16 dx=-2; dx<=2; dx++) {
          expanded.insert(&squares[square->y+dy][square->x+dx]);
        }
      }
    }
  }

  //TODO: move to private:
  void _all(u32 depth) {
    //Running at max_depth = 4 was 1m48s with checking and 2m18s without.
    //Need to test it for max_depth = 5
    if(/*depth > 4 or*/ not check_and_update_walked_set()) {
      walk();
      checked_board_counts[depth]++;
      if(depth < max_depth) {
        report_counts();
      }

      if(depth < max_depth) {
        std::unordered_set<Square *> expanded;
        _expand(expanded);
        for(Square * square : expanded) {
          if(square->val == 0) {
            push(square->x, square->y);
            _all(depth + 1);
            pop(square->x, square->y);
            refresh_visited_list();
          }
        }
      }
    }
  }

  void push(u16 x, u16 y) {
    Square * square = &(squares[y][x]);
    one_point_squares_list.one_point_squares_insert(square);

    square->cached_neighbor_sum = square->neighbor_sum;

    square->neighbor_sums_erase();
    square->neighbor_sum = 0;

    _push(x, y, 1);
    one_point_count++;
  }

  void pop(u16 x, u16 y) {
    Square * square = &squares[y][x];
    square->one_point_squares_erase();

    square->neighbor_sum = square->cached_neighbor_sum;

    _pop(x, y);
    one_point_count--;
  }

  void print(bool print_first_repr=true, bool print_all_reprs=false,
             bool print_neighbor_sum_lists=false) {
    u16 min_x, max_x, min_y, max_y;

    get_extents(min_x, min_y, max_x, max_y);
    printf("    y\\x|");
    for(u16 x=min_x-1; x<=max_x+1; x++) {
      printf("%7d|", x);
    }
    printf("\n");
    for(u16 y=min_y-1; y<=max_y+1; y++) {
      printf("%7d", y);
      for(u16 x=min_x-1; x<=max_x+1; x++) {
        printf("|");
        squares[y][x].print();
      }
      printf("|\n");
    }
    if(print_first_repr or print_all_reprs) {
      check_and_update_walked_set(true, true);
      if(print_all_reprs) {
        for(u32 i=0; i<8; i++) {
          printf("%s\n", packed_repr_buffs[i]);
        }
      } else {
        printf("%s\n", packed_repr_buffs[0]);
      }
    }
    if(print_neighbor_sum_lists) {
      for(u32 i=0; i<max_neighbor_sums; i++) {
        if(neighbor_sums_list[i].neighbor_sums_next != &neighbor_sums_ends[i]) {
          printf("%d: ", i);
          u32 count=0;
          ITERATE_INDEX(neighbor_sums, i, iter) {
            printf(" <%d,%d>", iter->x, iter->y);
            count++;
          }
          printf(" (%d squares)\n", count);
        }
      }
    }
  }

  void print_string_reprs() {
    check_and_update_walked_set(true, true);
    for(u32 i=0; i<8; i++) {
      printf("%d: %s\n", i, packed_repr_buffs[i]);
    }
    fflush(stdout);
  }
};

int main() {
  Board * board = new Board();

  /*
  board->push(100,102);
  board->push(104,100);
  board->push(106,102);
  board->walk();
  exit(0);
  */

  board->all();
  exit(0);
}
