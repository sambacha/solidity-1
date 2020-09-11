// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    int public x;

    constructor(int _x) { x = _x; }
}

contract BaseConstructorAndModifiers is A {
    modifier m {
        x++;
        _;
    }

    // Will call A() first, then the two modifiers
    constructor() m A(1) m {
        assert(x == 3);
    }

    receive() external payable { } // Needed for detecting as a truffle test case
}
