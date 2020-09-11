// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract A {
    int public x;

    /** @notice postcondition x == _x */
    function setX(int _x) public { x = _x; }
}

/** @notice invariant a.x() == y */
contract B {
    A a;
    int y;

    constructor() {
        y = 0;
        a.setX(y);
    }

    function incr() public {
        y++;
        a.setX(y);
    }

    function incr_incorrect() public {
        y++;
        a.setX(y+1);
    }
}