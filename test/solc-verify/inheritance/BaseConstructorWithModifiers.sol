// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract BCWM_A {
    int public x;

    modifier m(int _m) {
        x += _m;
        _;
    }

    constructor(int _x) m(_x) { x++; }
}

contract BaseConstructorWithModifiers is BCWM_A {

    constructor() BCWM_A(1) m(2) {
        assert(x == 4);
    }

    receive() external payable { } // Needed for detecting as a truffle test case
}
