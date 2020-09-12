// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract StructsMaps {

  struct A {
    int x;
    mapping(address=>int) m;
  }

  A a1;
  A a2;

  receive() external payable {
    assert(a1.x == 0);
    assert(a2.x == 0);
    assert(a1.m[address(this)] == 0);
    assert(a2.m[address(this)] == 0);

    a1.m[address(this)] = 2;
    a2.x = 3;

    assert(a1.x == 0);
    assert(a2.x == 3);
    assert(a1.m[address(this)] == 2);
    assert(a2.m[address(this)] == 0);
  }

}
