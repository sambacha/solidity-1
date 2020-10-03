// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract StructsArrays {

  struct A {
    int x;
    int[] m;
  }

  A a1;
  A a2;

  receive() external payable {
    // Conditions established by constructor
    require(a1.x == 0);
    require(a2.x == 0);
    require(a1.m.length == 0);
    require(a2.m.length == 0);

    A memory ma = A(1, new int[](2));
    ma.m[0] = -1; ma.m[1] = -2;
    assert(a1.x == 0);
    assert(a2.x == 0);
    assert(ma.x == 1);

    a1.m.push(0);
    a1.m.push(0);
    a1.m.push(0);
    ma.x = 2;
    a2.x = 3;

    a1 = ma;
    assert(a1.x == 2);
    assert(a1.m.length == 2);
    assert(a1.m[0] == -1);
    assert(a1.m[1] == -2);

    a1 = a2;
    assert(a1.x == 3);
    assert(a1.m.length == 0);
  }

}
