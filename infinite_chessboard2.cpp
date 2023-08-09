#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <list>
#include <string>
#include <thread>
#include <set>

#include "util.h"

#define MARK do{printf("%d\n", __LINE__); fflush(stdout);}while(0)

/*
KNOWN:
Stone Count | Best Score | Num Boards | Runtime
          2 |         16 | 5          |
          3 |         28 | 128        |
          4 |         38 | 7767       |     1m 02s
          5 |         49 | 501823     | 5h 32m 48s
          6 |         60 |
*/

#define DEFINE_INSERT(list_name) \
  void list_name##_insert(Square * square) { \
    if(square->list_name##_next != NULL) return; /*DON'T RE-INSERT!!!*/ \
    square->list_name##_next = this->list_name##_next; \
    square->list_name##_prev = this; \
    square->list_name##_next->list_name##_prev = square; \
    this->list_name##_next = square; \
  }
#define DEFINE_ERASE(list_name) \
  void list_name##_erase() { \
    if(this->list_name##_next == NULL) return; \
    this->list_name##_prev->list_name##_next = this->list_name##_next; \
    this->list_name##_next->list_name##_prev = this->list_name##_prev; \
    this->list_name##_prev = this->list_name##_next = NULL; \
  }
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
        printf("\033[31m");
      }
    }
    if(visited_next != NULL ) {
      printf("\033[4m");
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
#define ITERATE_INDEX(list_name, index, iterator_name) \
  for(Square * iterator_name = list_name##_list[index].list_name##_next; \
      iterator_name->list_name##_next != NULL; \
      iterator_name = iterator_name->list_name##_next)

class Board {
private:
  static const u32 board_size = 1001; // MUST BE ODD (so refletion works)
  static const u32 board_mid = board_size/2;
  static const u32 max_neighbor_sums = 2000; //Paying memory for safety/speed.
  static const u16 max_depth = 4;
  static const u32 buf_len = 16*(max_depth + 1); //A generous estimate.

  Square squares[board_size][board_size];
  char packed_repr_buffs[8][buf_len];
  std::set<std::string> walked_boards;
  u16 best_scores[max_depth + 1];
  std::vector<std::string> best_solutions;
  u64 total_board_counts[9] = {0, 0, 5, 128, 7767, 501823, 0, 0, 0};
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

  double start_time;

  s16 dydx[8][2] = {
    {-1,-1}, {-1, 0}, {-1, 1},
    { 0,-1},          { 0, 1},
    { 1,-1}, { 1, 0}, { 1, 1}};

  void get_visited_extents(u16 & min_x, u16 & min_y, u16 & max_x, u16 & max_y) {
    min_x = u16_max, max_x = u16_min;
    min_y = u16_max, max_y = u16_min;

    ITERATE(visited, iter) {
      min_x = std::min(min_x, iter->x);
      max_x = std::max(max_x, iter->x);
      min_y = std::min(min_y, iter->y);
      max_y = std::max(max_y, iter->y);
    }
  }

  void get_one_point_extents(u16 & min_x, u16 & min_y, u16 & max_x, u16 & max_y) {
    min_x = u16_max, max_x = u16_min;
    min_y = u16_max, max_y = u16_min;

    ITERATE(one_point_squares, iter) {
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

    get_one_point_extents(min_x, min_y, max_x, max_y);
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
    // The order that we visit squares tends to be around the one-pointers first,
    // then moving outward. The density of possible paths to trace is considerably
    // higher near the one-pointers than around the perimeter. If I create this
    // copy in the same order that the original is created, the rate at which
    // computation is done will either accelerate or decelerate, depending on the
    // order. This makes eta estimation very difficult.
    //
    // It's still terrible, but it's an improvement.
    bool flipflop = true;
    // I hate doing this copy. I'd love to find a way to skip it.
    std::vector<Square *> neighbor_sums_equal_to_val;
    ITERATE_INDEX(neighbor_sums, val, iter) {
      if(flipflop) {
        neighbor_sums_equal_to_val.push_back(iter);
      } else {
        //neighbor_sums_equal_to_val.push_front(iter);
      }
      //flipflop = ! flipflop;
    }

    if(neighbor_sums_equal_to_val.size() > 0) {
      if(val > best_scores[one_point_count]) {
        best_scores[one_point_count] = val;
        printf("New best (%d stones): %d\n", one_point_count, val);
        print(true, false, false);
        best_solutions[one_point_count] = packed_repr_buffs[0];
        printf("\n");
      }
      for(Square * square : neighbor_sums_equal_to_val) {
        _push(square->x, square->y, val);
        _walk(val+1);
        _pop(square->x, square->y);
      }
    }
  }

  void refresh_visited_list() {
    while(visited_list.visited_next != &visited_end) {
      visited_list.visited_next->visited_erase();
    }

    /*
    if(visited_list.visited_next != &visited_end) {
      printf("Fatal %d\n", __LINE__); exit(1);
    }
    if(visited_end.visited_prev != &visited_list) {
      printf("Fatal %d\n", __LINE__); exit(1);
    }
    if(visited_list.visited_prev != NULL) {
      printf("Fatal %d\n", __LINE__); exit(1);
    }
    if(visited_end.visited_next != NULL) {
      printf("Fatal %d\n", __LINE__); exit(1);
    }
    */

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

  void report_counts(bool force=true) {
    if(!force) {
      return;
    }
    printf("\n");
    for(u32 i=2; i<=max_depth; i++) {
      printf("%d stone best: %d, checked: %lu/%lu, solution: %s\n",
          i, best_scores[i], checked_board_counts[i], total_board_counts[i],
          best_solutions[i].c_str());
    }

    u32 compute_on = (u16)5 < max_depth ? 5 : max_depth;
    //Throws an inexplicable linker error on max_depth:
    //u32 compute_on = std::min((u16)5, max_depth); // 5 is the best we have counts for.
    double fraction_completed =
        (double)checked_board_counts[compute_on]/total_board_counts[compute_on];
    double elapsed_time = now() - start_time;
    u32 elapsed_min = elapsed_time/60;
    u32 elapsed_sec = ((u32)elapsed_time)%60;
    double eta = elapsed_time/fraction_completed - elapsed_time;
    u32 eta_min = eta/60;
    u32 eta_sec = ((u32)eta)%60;
    printf("[%6.4f%%] in %dm%02ds. Eta: %dm%02ds\n", fraction_completed*100.0,
        elapsed_min, elapsed_sec, eta_min, eta_sec);
    fflush(stdout);
  }

public:
  Board() :
      one_point_count(0)
  {
    start_time = now();
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
        if(dx || dy) {
          push(board_mid + dx, board_mid + dy);
          _all(2);
          pop(board_mid + dx, board_mid + dy);
        }
      }
    }
    report_counts(true);
  }

  //TODO: move to private:
  void _expand(std::set<Square *> & expanded) {
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
    if(/*depth > 4 or*/ not check_and_update_walked_set()) {
      if(depth < max_depth) {
        //The visited list won't be used for depth < max depth, so don't
        //go through the overhead of clearing it.
        refresh_visited_list();
      }
      walk();
      checked_board_counts[depth]++;
      if(depth < max_depth) {
        report_counts();

        std::set<Square *> expanded;
        _expand(expanded);
        for(Square * square : expanded) {
          if(square->val == 0) {
            push(square->x, square->y);
            _all(depth + 1);
            pop(square->x, square->y);
          }
        }
      }
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

  void print(bool print_first_repr=true, bool print_all_reprs=false,
             bool print_neighbor_sum_lists=false, bool force=true) {
    if(!force) {
      return;
    }

    u16 min_x_visited, max_x_visited, min_y_visited, max_y_visited;
    u16 min_x_active = u16_max, max_x_active = u16_min,
        min_y_active = u16_max, max_y_active = u16_min;

    get_visited_extents(
        min_x_visited, min_y_visited, max_x_visited, max_y_visited);

    for(u16 y=min_y_visited; y<=max_y_visited; y++) {
      for(u16 x=min_x_visited; x<=max_x_visited; x++) {
        if(squares[y][x].val) {
          min_x_active = std::min(min_x_active, x);
          min_y_active = std::min(min_y_active, y);
          max_x_active = std::max(max_x_active, x);
          max_y_active = std::max(max_y_active, y);
        }
      }
    }

    printf("    y\\x|");
    for(u16 x=min_x_active-1; x<=max_x_active+1; x++) {
      printf("%7d|", x);
    }
    printf("\n");
    for(u16 y=min_y_active-1; y<=max_y_active+1; y++) {
      printf("%7d", y);
      for(u16 x=min_x_active-1; x<=max_x_active+1; x++) {
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

class ArgParse {
private:
  void usage(s32 exit_val) {
    fflush(stderr);
    printf("usage: infchess max_depth -c -a=remote_addr -p=port_number|\n");
    printf("       infchess max_depth -s -p=port_number\n");
    printf("       infchess max_depth\n\n");
    printf("The first form creates a worker client and connects to the\n");
    printf("server at the remote_addr and port_numer given\n\n");
    printf("The second form creates an orchestrator process to which\n");
    printf("the clients will connect.\n");
    printf("The final form creates a local-only process\n");
    exit(exit_val);
  }
public:
  ArgParse(s32 argc, char * argv[]) :
      m_client(false),
      m_server(false),
      m_standalone(true),
      m_port(0),
      m_max_depth(0),
      m_remote_address(NULL)
  {
    if(argc < 2) {
      usage(1);
    }

    m_max_depth = atoi(argv[1]);
    if(m_max_depth == 0) {
      fprintf(stderr, "Unable to parse max_depth\n");
      usage(1);
    }
    for(s32 i=2; i<argc; i++) {
      if(argv[i][0] != '-') {
        usage(1);
      }
      switch(argv[i][1]) {
        case 'h':
        case '?':
          usage(0);
          break;
        case 's':
          if(m_client) {
            fprintf(stderr, "-s and -c are mutually exclusive\n");
            usage(1);
          }
          m_server = true;
          m_standalone = false;
          break;
        case 'c':
          if(m_server) {
            fprintf(stderr, "-s and -c are mutually exclusive\n");
            usage(1);
          }
          m_client = true;
          m_standalone = false;
          break;
        case 'a':
          if(argv[i][2] != '=') {
            usage(1);
          }
          m_remote_address = &argv[i][3];
          break;
        case 'p':
          if(argv[i][2] != '=') {
            usage(1);
          }
          m_port=atoi(&argv[i][3]);
          break;
      }
    }

    if((m_server || m_client) && m_port == 0) {
      fprintf(stderr, "Port num required when starting a client or server.\n");
      usage(1);
    }

    if(m_client && m_remote_address == NULL) {
      fprintf(stderr, "Remote IP is required when starting a client.\n");
      usage(1);
    }
  }

  bool m_client;
  bool m_server;
  bool m_standalone;

  u16 m_max_depth;

  u16 m_port;
  char * m_remote_address;
};

int main(s32 argc, char * argv[]) {
  Board * board = new Board();

  ArgParse args(argc, argv);
  exit(0);
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
