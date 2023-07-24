#!/usr/bin/env python3

from collections import defaultdict

debug = False

class Pos(object):
  def __init__(self, x, y):
    self.x = x
    self.y = y

  def __add__(self, dxdy):
    return Pos(self.x + dxdy[0], self.y + dxdy[1])

  def __eq__(self, other):
    return self.x == other.x and self.y == other.y
  def __hash__(self):
    return hash(str(self))
  def __str__(self):
    return f'<{self.x},{self.y}>'
  def __repr__(self):
    return f'<{self.x},{self.y}>'

class Node(object):
  def __init__(self):
    self.val = 0
    self.neighbor_sum = 0

  def __repr__(self):
    return f'[{self.val}, {self.neighbor_sum}]'

class Board(object):
  #The value is [Score of stone here (if exists), value of surroundings]

  def __init__(self):
    self.squares = defaultdict(Node)
    self.neighbor_sums = defaultdict(set)

  def push(self, pos, val):
    if debug:
      print('push(' + str(pos) + ',' + str(val) + ')')
    self.squares[pos].val = val
    for dxdy in [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]:
      neighbor = pos + dxdy
      if self.squares[neighbor].val == 0:
        oldsum = self.squares[neighbor].neighbor_sum
        newsum = oldsum + val
        self.squares[neighbor].neighbor_sum = newsum
        if oldsum > 0:
          self.neighbor_sums[oldsum].remove(neighbor)
        self.neighbor_sums[newsum].add(neighbor)
    if debug:
      print()
      self.print()
      print()
      print()

  def pop(self, pos):
    if debug:
      print('pop(' + str(pos) + ')')
    val = self.squares[pos].val
    self.squares[pos].val = 0
    for dxdy in [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]:
      neighbor = pos + dxdy
      if self.squares[neighbor].val == 0:
        oldval = self.squares[neighbor].neighbor_sum
        newval = oldval - val
        self.squares[neighbor].neighbor_sum = newval
        try:
          self.neighbor_sums[oldval].remove(neighbor)
        except:
          pass
        if newval > 0:
          self.neighbor_sums[newval].add(neighbor)
    if debug:
      print()
      self.print()
      print()
      print()

  def print(self):
    minx = 999999999
    miny = 999999999
    maxx = -minx
    maxy = -miny
    for k,v in self.squares.items():
      if v.val != 0:
        if k.x > maxx:
          maxx = k.x
        if k.x < minx:
          minx = k.x
        if k.y > maxy:
          maxy = k.y
        if k.y < miny:
          miny = k.y

    for yy in range(miny-1, maxy+2):
      for xx in range(minx-1, maxx+2):
        xy = Pos(xx,yy)
        if debug:
          print('|%3d,%3d' % (
            self.squares[xy].val,
            self.squares[xy].neighbor_sum), end='')
        else:
          print('|%3d' % self.squares[xy].val, end = '')
      print('|')

    if debug:
      sum_keys = list(self.neighbor_sums.keys())
      sum_keys.sort()
      for key in sum_keys:
        print(f'{key}({len(self.neighbor_sums[key])}): {self.neighbor_sums[key]}')

  def _walk(self, val):
    positions = list(self.neighbor_sums[val])
    for pos in positions:
      self.push(pos, val)
      #TODO: figure out how to do this check only once
      if val > self.best:
        self.best_count = 1
        self.best = val
        print('\n New Best:', val)
        self.print()
      self._walk(val+1)
      self.pop(pos)

  def walk(self):
    self.best = 1
    self._walk(2)

board = Board()
board.push(Pos(0,0),1)
board.push(Pos(4,2),1)
board.push(Pos(6,4),1)
board.push(Pos(9,2),1)

board.walk()
