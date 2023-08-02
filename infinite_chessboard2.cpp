#include <stdio.h>

#include <algorithm>
#include <string>
#include <unordered_set>

#include "util.h"

#define MARK do{printf("%d\n", __LINE__); fflush(stdout);}while(0)

#define DEFINE_INSERT(list_name) \
  void list_name ## _insert(Square * node) { \
    if(node->list_name##_next != NULL) return; /*DON'T RE-INSERT!!!*/ \
    node->list_name##_next = this->list_name##_next; \
    node->list_name##_prev = this; \
    node->list_name##_next->list_name##_prev = node; \
    this->list_name##_next = node; \
  }
#define DEFINE_ERASE(list_name) \
  void list_name##_erase() { \
    if(this->list_name##_prev == NULL) return; \
    this->list_name##_prev->list_name##_next = this->list_name##_next; \
    this->list_name##_next->list_name##_prev = this->list_name##_prev; \
    this->list_name##_prev = this->list_name##_next = NULL; \
  }
#define ITERATE(list_name, iterator_name) \
  for(Square * iterator_name = list_name##_list.list_name##_next; \
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
  Square * one_point_squares_next;
  Square * one_point_squares_prev;
  Square * visited_next;
  Square * visited_prev;

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
      one_point_squares_next(NULL)
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


class Board {
private:
  static const u32 board_size = 15; // MUST BE ODD (so refletion works)
  static const u32 board_mid = board_size/2;
  static const u32 max_neighbor_sums = 100000; //Paying memory for safety/speed.
  static const u16 max_depth = 4;
  static const u32 buf_len = 16*(max_depth + 1); //A generous estimate.

  Square squares[board_size][board_size];
  char compact_repr_buffs[8][buf_len];

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
          square->neighbor_sums_erase();
        }
        neighbor_sums_list[new_sum].neighbor_sums_insert(square);
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

  inline u32 compact(u16 x, u16 y, u16 min_x, u16 min_y) {
    return ((y-min_y)<<16) + (x-min_x);
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

  void generate_string_reprs(bool do_seven = true) {
    u32 repr_list[8][max_depth];
    u32 repr_list_next=0;
    u16 min_x, min_y, max_x, max_y;

    get_extents(min_x, min_y, max_x, max_y);
    u16 span_x = 1 + max_x - min_x;
    u16 span_y = 1 + max_y - min_y;
    ITERATE(one_point_squares, iter) {
      u32 x = (u32)(iter->x);
      u32 y = (u32)(iter->y);

      repr_list[0][repr_list_next++] = compact(x, y, min_x, min_y);
    }

    u32_to_buf(
      compact_repr_buffs[0], repr_list[0], repr_list_next, span_x, span_y);

    //TODO: If the first one is already in the cache, quit early.

    //As the saying goes, the algorithm to do this is very nasty. In fact,
    //you might want to mug someone with it.

    if(not do_seven) {
      return;
    }

    u32 repr_list_next = 0;
    u16 min_x_inv = board_size - min_x - 1;
    u16 min_y_inv = board_size - min_y - 1;
    u16 max_x_inv = board_size - max_x - 1;
    u16 max_y_inv = board_size - max_y - 1;
    ITERATE(one_point_squares_list, iter) {
      x = iter->x;
      y = iter->y;
      //We've already done [y][x], so we output nothing to repr_list[0].
      //Let's reflect about y=board_mid:
      y = board_size - y - 1;
      repr_list[1][repr_list_next] = compact(x, y, x_min, max_y_inv);
      //Reflect about x=board_mid axis:
      x = board_size - x - 1;
      repr_list[2][repr_list_next] = compact(x, y, max_x_inv, max_y_inv);
      //Reflect about y=board_mid again:
      y = board_size - y - 1;
      repr_list[3][repr_list_next] = compact(x, y, max_x_inv, min_y);

      //Now we do a reflection about x=y:
      std::swap(x, y);
      repr_list[4][repr_list_next] = compact(x, y, max_y_inv, min_x);

      //Reflect about x=board_mid (continuing clockwise.):
      x = board_size - x - 1;
      repr_list[5][repr_list_next] = compact(x, y, min_y, min_x);
      //Reflect about y=board_mid
      y = board_size - y - 1;
      repr_list[6][repr_list_next] = compact(x, y, min_y, max_x_inv);
      //Reflect about x=board_mid
      x = board_size - x - 1;
      repr_list[6][repr_list_next] = compact(x, y, max_y_inv, max_x_inv);

      repr_list_next++;
    }
    for(u32 i=1; i<8; i++) {
      if(i<4){
        u32_to_buf(
          compact_repr_buffs[1], repr_list[1], repr_list_next, span_x, span_y);
      } else {
        u32_to_buf(
          compact_repr_buffs[1], repr_list[1], repr_list_next, span_y, span_x);
      }
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

    visited_list.visited_next = &(visited_end);
    visited_end.visited_prev = &(visited_list);

    one_point_squares_list.one_point_squares_next = &one_point_squares_end;
    one_point_squares_end.one_point_squares_prev = &one_point_squares_list;

    for(u32 i=0; i<max_neighbor_sums; i++) {
      neighbor_sums_list[i].neighbor_sums_next = &(neighbor_sums_ends[i]);
      neighbor_sums_ends[i].neighbor_sums_prev = &(neighbor_sums_list[i]);
    }
  }

  void push(u16 x, u16 y) {
    Square * square = &(squares[y][x]);
    one_point_squares_list.one_point_squares_insert(square);
    one_point_count++;

    square->cached_neighbor_sum = square->neighbor_sum;

    square->neighbor_sums_erase();
    square->neighbor_sum = 0;

    _push(x, y, 1);
  }

  void pop(u16 x, u16 y) {
    Square * square = &squares[y][x];
    square->one_point_squares_erase();
    one_point_count--;

    square->neighbor_sum = square->cached_neighbor_sum;

    _pop(x, y);
  }

  void print(bool print_first_repr=true, bool print_all_reprs=false) {
    u16 min_x, max_x, min_y, max_y;

    get_extents(min_x, min_y, max_x, max_y);
    for(u16 y=min_y-1; y<=max_y+1; y++) {
      for(u16 x=min_x-1; x<=max_x+1; x++) {
        printf("|");
        squares[y][x].print();
      }
      printf("|\n");
    }
    if(print_first_repr or print_all_reprs) {
      generate_string_reprs(true);
      if(print_all_reprs) {
        for(u32 i=0; i<8; i++) {
          printf("%s\n", compact_repr_buffs[i]);
        }
      } else {
        printf("%s\n", compact_repr_buffs[0]);
      }
    }
  }
};

int main() {
  Board * board = new Board;

  board->push(1,10);
  board->push(1, 9);
  board->push(2, 8);
  board->print();
}
