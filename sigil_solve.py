#!/usr/bin/env python

import sys


def usage():
    print("sigil_solve.py <width> <height> <block ID> ...")
    print("Available blocks are...")
    print
    for i in range(len(block_types)):
        print("ID:", i, "\nName:", block_types[i][0], "Shape:")
        print_block_shape(block_types[i][1])
        print
    exit()


def print_block_shape(b):
    for r in b:
        for c in r:
            if c == 1:
                sys.stdout.write("#")
            else:
                sys.stdout.write(" ")
        sys.stdout.write("\n")


def print_board(b):
    for r in b:
        for c in r:
            if c == 0:
                sys.stdout.write("-")
            else:
                sys.stdout.write(chr(ord('A') + c - 1))
                #sys.stdout.write(str(c))
        sys.stdout.write("\n")


def rotate_right(b):
    rb = [[-1 for x in range(len(b))] for x in range(len(b[0]))]
    for x in range(len(b[0])):
        for y in range(len(b)):
            rb[x][len(b) - y - 1] = b[y][x]
    return rb


# n: number of times to rotate the block right
def rotate_block(b, n):
    for i in range(n):
        b = rotate_right(b)
    return b


def add_block(board, block, x, y, scale=1):
    for i in range(len(block[0])):
        for j in range(len(block)):
            board[j + y][i + x] += scale * block[j][i]


def remove_block(board, block, x, y, scale=1):
    for i in range(len(block[0])):
        for j in range(len(block)):
            board[j + y][i + x] -= scale * block[j][i]


def test_position(board, block, x, y):
    for i in range(len(block[0])):
        for j in range(len(block)):
            if block[j][i] * board[j + y][i + x] > 0:
                return False
    return True


def valid_positions(board, block):
    # Look at
    pos = []
    for i in range(len(board[0]) - len(block[0]) + 1):
        for j in range(len(board) - len(block) + 1):
            if test_position(board, block, i, j):
                pos.append((i, j))
    return pos


def valid_states(board, block):
    b = block
    states = [[b, valid_positions(board, b)]]
    for i in range(1, 4):
        b = rotate_right(b)
        if b in [s[0] for s in states]:
            break
        states.append([b, valid_positions(board, b)])
    return states


def place_block(board, blocks, n):
    global num_configs
    for s in valid_states(board, blocks[0]):
        b = s[0]
        for p in s[1]:
            add_block(board, b, p[0], p[1], n)
            num_configs += 1
            if len(blocks) > 1:
                ret = place_block(board, blocks[1:], n + 1)
                if ret == True:
                    return True
                else:
                    remove_block(board, b, p[0], p[1], n)
            else:
                return True
    return False


num_configs = 0

block_types = [["RightStep", [[1, 1, 0], [0, 1, 1]]],
               ["LeftStep", [[0, 1, 1], [1, 1, 0]]],
               ["L", [[1, 0], [1, 0], [1, 1]]],
               ["BackwardsL", [[0, 1], [0, 1], [1, 1]]],
               ["T", [[0, 1, 0], [1, 1, 1]]], ["Square", [[1, 1], [1, 1]]],
               ["Line", [
                   [1, 1, 1, 1],
               ]]]

if len(sys.argv) < 4:
    usage()

width = int(sys.argv[1])
height = int(sys.argv[2])

block_ids = [int(b) for b in sys.argv[3:]]

for bid in block_ids:
    if bid not in range(len(block_types)):
        print(bid, "is not a valid block ID")
        exit(1)

blocks = [block_types[b][1] for b in block_ids]

# Make sure that block IDs are OK, and the sum of the sizes matches the board
s = 0
for b in blocks:
    s += sum([sum(r) for r in b])

if s != width * height:
    print("Chosen blocks (" + str(s) + " squares) won't fill the board (" +
          str(width * height) + " squares)")
    exit(2)

board = [[0 for c in range(width)] for r in range(height)]

place_block(board, blocks, 1)
print_board(board)
print(num_configs, "configurations tested")
