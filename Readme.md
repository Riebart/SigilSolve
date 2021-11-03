# Talos Principle Sigil Solver

A trivial brute-force depth-first solver for finding a tetromino layout that covers a rectangular board.

For the Python, run without arguments for it to print out the ID to tetromino mapping and usage:

```bash
$ python3 sigil_solve.py
sigil_solve.py <width> <height> <block ID> ...
Available blocks are...
ID: 0
Name: RightStep Shape:
##
 ##
ID: 1
Name: LeftStep Shape:
 ##
##
ID: 2
Name: L Shape:
#
#
##
ID: 3
Name: BackwardsL Shape:
 #
 #
##
ID: 4
Name: T Shape:
 #
###
ID: 5
Name: Square Shape:
##
##
ID: 6
Name: Line Shape:
####
```

## Example

Given the below board (4 wide, 6 tall) and tetromino set:

![Example tetromino set](https://i.ytimg.com/vi/YXS5Zb21Nc0/maxresdefault.jpg)

Find a solution for tiling the board with the given tiles:

```bash
$ python3 sigil_solve.py 4 6 0 3 4 4 5 6
EEBB
EEBF
AABF
DAAF
DDCF
DCCC
1497 configurations tested
```
