// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  struct S {
    string x;
    int[] y;
  }
  mapping (int=>S) m;
  function f() public {
    m[42].y[0] = 5;
    S memory mem = m[42];
    assert(mem.y[0] == 5);
  }
}
