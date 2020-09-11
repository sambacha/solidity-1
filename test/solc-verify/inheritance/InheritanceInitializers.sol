// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    int x = 1;
    int y;
}

contract B is A {
    int z = 3;
    int t;

    constructor() {
        assert(x == 1);
        assert(y == 0);
        assert(z == 3);
        assert(t == 0);
    }
}