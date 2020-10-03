// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract StructsSwap {

  struct A {
    int x;
  }

  A a1;
  A a2;
  A a3;

  struct B {
    A x;
    A y;
  }

  B b;

  receive() external payable {
    a1.x = 1;
    a2.x = 2;
    a3.x = 3;
    assert(a1.x == 1);
    assert(a2.x == 2);
    assert(a3.x == 3);

    (a1, a3, a2) = (a3, a2, a1);
    assert(a1.x == 1);
    assert(a2.x == 1);
    assert(a3.x == 1);

    a1.x = 1;
    a2.x = 2;
    a3.x = 3;
    (a1.x, a3.x, a2.x) = (a3.x, a2.x, a1.x);
    assert(a1.x == 3);
    assert(a2.x == 1);
    assert(a3.x == 2);

    a1.x = 1;
    a2.x = 2;
    a3.x = 3;
    (a1.x, a3, a2.x) = (a3.x, a2, a1.x);
    assert(a1.x == 3);
    assert(a2.x == 1);
    assert(a3.x == 1);

    b.x.x = 1;
    b.y.x = 2;
    (b.x, b.y) = (b.y, b.x);
    assert(b.x.x == 1);
    assert(b.y.x == 1);
  }

}
