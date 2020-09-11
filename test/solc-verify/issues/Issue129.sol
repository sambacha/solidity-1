// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
  struct S {
    int x;
  }
}
contract B {
  function f() public pure returns (int) {
    A.S memory s = A.S(0);
    return s.x;
  }
}