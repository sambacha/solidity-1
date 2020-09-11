// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {
    int x;
    constructor(int _x) { x = _x; }
}

contract D is C(1) {
    constructor() {
        assert(x == 1);
    }
}

contract E is C {
    constructor() C(1) public {
        assert(x == 1);
    }
}
