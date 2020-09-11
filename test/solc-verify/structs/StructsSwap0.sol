// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract StructsSwap0 {

  struct A {
    int x;
  }

  A a1;
  A a2;

  receive() external payable {
    a1.x = 1;
    a2.x = 2;
    assert(a1.x == 1);
    assert(a2.x == 2);

    (a1, a2) = (a2, a1);
    assert(a1.x == 1);
    assert(a2.x == 1);
  }
}
