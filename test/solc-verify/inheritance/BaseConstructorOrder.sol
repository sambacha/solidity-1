// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract BCO_A {
    int public x;
    /// @notice postcondition (x == __verifier_old_int(_x))
    function set(int _x) public { x = _x; }
    constructor(int _x) { set(_x); }
}

contract BCO_B1 is BCO_A {
  constructor(int _x) BCO_A(_x) { set(x*_x); }
}

abstract contract BCO_B2 is BCO_A {
  constructor(int _x) { set(x+_x); }
}

contract BaseConstructorOrder is BCO_B1, BCO_B2 {

  modifier m(int _x) {
    _;
    set(_x);
    assert(x == 6);
  }

  constructor() BCO_B1(x+2) m(x+2) BCO_B2(x) {
    assert(x == 4);
  }

  receive() external payable { } // Needed for detecting as a truffle test case
}
