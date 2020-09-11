// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Initializers {
    int b;
    int c;

    constructor(int b0) {
        assert(a == 5);
        assert(b == 0);
        assert(c == 0);
        b = b0;
    }

    int a = 5;
}

/**
 * @notice invariant a == 5
 * @notice invariant b == 0
 */
contract NoConstructorButNeeded1 {
    int a = 5;
    int b;
}

/**
 * @notice invariant a == 0
 * @notice invariant b == 0
 */
contract NoConstructorButNeeded2 {
    int a;
    int b;
}